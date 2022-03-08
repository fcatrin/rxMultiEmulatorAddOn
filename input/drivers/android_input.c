/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2010-2014 - Hans-Kristian Arntzen
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *  Copyright (C) 2012-2015 - Michael Lelli
 *  Copyright (C) 2013-2014 - Steven Crowe
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include <android/keycodes.h>
#include <unistd.h>
#include <dlfcn.h>

#include <retro_inline.h>

#include "../../frontend/drivers/platform_android.h"
#include "../input_autodetect.h"
#include "../input_common.h"
#include "../input_joypad.h"
#include "../../performance.h"
#include "../../general.h"
#include "../../driver.h"
#include "../../system.h"

#define MAX_TOUCH 16
#define MAX_PADS 8

#define AKEY_EVENT_NO_ACTION 255
#define ANALOG_DIGITAL_THRESHOLD 0.2f

#ifndef AKEYCODE_ASSIST
#define AKEYCODE_ASSIST 219
#endif

#define LAST_KEYCODE AKEYCODE_ASSIST

#define AMOTION_EVENT_ACTION_HOVER_MOVE  7
#define AMOTION_EVENT_ACTION_HOVER_ENTER 9
#define AMOTION_EVENT_ACTION_HOVER_EXIT  10
#define RETRO_DEVICE_ID_MOUSE_HOVER      16

enum {
	COMBO_REWIND_FORWARD_SIMPLE,
	COMBO_REWIND_FORWARD_SELECT,
	COMBO_REWIND_FORWARD_R3,
};

typedef struct
{
   float x;
   float y;
   float z;
} sensor_t;

struct input_pointer
{
   int16_t x, y;
   int16_t full_x, full_y;
};

enum
{
   AXIS_X = 0,
   AXIS_Y = 1,
   AXIS_Z = 11,
   AXIS_RZ = 14,
   AXIS_HAT_X = 15,
   AXIS_HAT_Y = 16,
   AXIS_LTRIGGER = 17,
   AXIS_RTRIGGER = 18,
   AXIS_GAS = 22,
   AXIS_BRAKE = 23
};

#define MAX_AXIS 10

typedef struct state_device
{
   int id;
   int port;
   int ignore_back;
   char name[256];
   char descriptor[256];
   int is_nvidia;
} state_device_t;

typedef struct android_input
{
   bool blocked;
   unsigned pads_connected;
   state_device_t pad_states[MAX_PADS];
   uint8_t pad_state[MAX_PADS][(LAST_KEYCODE + 7) / 8];
   int8_t  hat_state[MAX_PADS][2];
   int8_t dpad_state[MAX_PADS][2];
   int8_t  trigger_state[MAX_PADS][2];
   bool mame_trigger_state_l2;
   bool mame_trigger_state_r2;
   bool mame_trigger_state_l3;
   bool mame_trigger_state_r3;
   bool    motion_from_hover;
   int8_t  mouse_button_click;
   struct timeval mouse_button_click_start;
   struct timeval mouse_button_click_stop;

   int16_t analog_state[MAX_PADS][MAX_AXIS];
   sensor_t accelerometer_state;
   struct input_pointer pointer[MAX_TOUCH];
   unsigned pointer_count;
   ASensorManager *sensorManager;
   ASensorEventQueue *sensorEventQueue;
   const input_device_driver_t *joypad;
   bool is_back_pressed;

   int16_t joypad_0_select_keycode;
   int16_t joypad_0_r3_keycode;

   bool joypad_0_select_pressed;
   bool joypad_0_r3_pressed;

} android_input_t;

static void frontend_android_get_version_sdk(int32_t *sdk);

bool (*engine_lookup_name)(char *buf,
      int *vendorId, int *productId, size_t size, int id);

void (*engine_handle_dpad)(android_input_t *android, AInputEvent*, int, int);
static bool android_input_set_sensor_state(void *data, unsigned port,
      enum retro_sensor_action action, unsigned event_rate);

extern float AMotionEvent_getAxisValue(const AInputEvent* motion_event,
      int32_t axis, size_t pointer_idx);

static typeof(AMotionEvent_getAxisValue) *p_AMotionEvent_getAxisValue;

#define AMotionEvent_getAxisValue (*p_AMotionEvent_getAxisValue)

static void engine_handle_dpad_default(android_input_t *android,
      AInputEvent *event, int port, int source)
{
   size_t motion_ptr = AMotionEvent_getAction(event) >>
      AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
   float x           = AMotionEvent_getX(event, motion_ptr);
   float y           = AMotionEvent_getY(event, motion_ptr);

   android->analog_state[port][0] = (int16_t)(x * 32767.0f);
   android->analog_state[port][1] = (int16_t)(y * 32767.0f);
}

static void engine_handle_dpad_real(android_input_t *android, int port)
{
	   // translate dpad into hat
	   uint8_t *buf = android->pad_state[port];

	   int dpadx = 0;
	   int dpady = 0;

	   if (BIT_GET(buf, AKEYCODE_DPAD_UP)) {
		   dpady = -1;
	   } else if (BIT_GET(buf, AKEYCODE_DPAD_DOWN)) {
		   dpady = 1;
	   }

	   if (BIT_GET(buf, AKEYCODE_DPAD_LEFT)) {
		   dpadx = -1;
	   } else if (BIT_GET(buf, AKEYCODE_DPAD_RIGHT)) {
		   dpadx = 1;
	   }

	   android->dpad_state[port][0] = dpadx;
	   android->dpad_state[port][1] = dpady;

}

static void engine_handle_dpad_getaxisvalue(android_input_t *android,
      AInputEvent *event, int port, int source)
{
   size_t motion_ptr = AMotionEvent_getAction(event) >>
      AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
   float x           = AMotionEvent_getAxisValue(event, AXIS_X, motion_ptr);
   float y           = AMotionEvent_getAxisValue(event, AXIS_Y, motion_ptr);
   float z           = AMotionEvent_getAxisValue(event, AXIS_Z, motion_ptr);
   float rz          = AMotionEvent_getAxisValue(event, AXIS_RZ, motion_ptr);
   float hatx        = AMotionEvent_getAxisValue(event, AXIS_HAT_X, motion_ptr);
   float haty        = AMotionEvent_getAxisValue(event, AXIS_HAT_Y, motion_ptr);
   float ltrig       = AMotionEvent_getAxisValue(event, AXIS_LTRIGGER, motion_ptr);
   float rtrig       = AMotionEvent_getAxisValue(event, AXIS_RTRIGGER, motion_ptr);
   float brake       = AMotionEvent_getAxisValue(event, AXIS_BRAKE, motion_ptr);
   float gas         = AMotionEvent_getAxisValue(event, AXIS_GAS, motion_ptr);

   android->hat_state[port][0] = (int)hatx;
   android->hat_state[port][1] = (int)haty;

   /* XXX: this could be a loop instead, but do we really want to
    * loop through every axis? */
   android->analog_state[port][0] = (int16_t)(x * 32767.0f);
   android->analog_state[port][1] = (int16_t)(y * 32767.0f);
   android->analog_state[port][2] = (int16_t)(z * 32767.0f);
   android->analog_state[port][3] = (int16_t)(rz * 32767.0f);
#if 0
   android->analog_state[port][4] = (int16_t)(hatx * 32767.0f);
   android->analog_state[port][5] = (int16_t)(haty * 32767.0f);
#endif
   android->analog_state[port][6] = (int16_t)(ltrig * 32767.0f);
   android->analog_state[port][7] = (int16_t)(rtrig * 32767.0f);
   android->analog_state[port][8] = (int16_t)(brake * 32767.0f);
   android->analog_state[port][9] = (int16_t)(gas * 32767.0f);

   android->trigger_state[port][0] = brake > ANALOG_DIGITAL_THRESHOLD || ltrig > ANALOG_DIGITAL_THRESHOLD;
   android->trigger_state[port][1] = gas   > ANALOG_DIGITAL_THRESHOLD || rtrig > ANALOG_DIGITAL_THRESHOLD;

}

static bool android_input_lookup_name_prekitkat(char *buf,
      int *vendorId, int *productId, size_t size, int id)
{
   jobject name      = NULL;
   jmethodID getName = NULL;
   jobject device    = NULL;
   jmethodID method  = NULL;
   jclass    class   = 0;
   const char *str   = NULL;
   JNIEnv     *env   = (JNIEnv*)jni_thread_getenv();
   bool result = false;

   if (!env)
      goto error;

   RARCH_LOG("Using old lookup");

   FIND_CLASS(env, class, "android/view/InputDevice");
   if (!class)
      goto error;

   GET_STATIC_METHOD_ID(env, method, class, "getDevice",
         "(I)Landroid/view/InputDevice;");
   if (!method)
      goto error;

   CALL_OBJ_STATIC_METHOD_PARAM(env, device, class, method, (jint)id);
   if (!device)
   {
      RARCH_ERR("Failed to find device for ID: %d\n", id);
      goto error;
   }

   GET_METHOD_ID(env, getName, class, "getName", "()Ljava/lang/String;");
   if (!getName)
      goto error;

   CALL_OBJ_METHOD(env, name, device, getName);
   if (!name)
   {
      RARCH_ERR("Failed to find name for device ID: %d\n", id);
      goto error;
   }

   buf[0] = '\0';

   str = (*env)->GetStringUTFChars(env, name, 0);
   if (str)
      strlcpy(buf, str, size);
   (*env)->ReleaseStringUTFChars(env, name, str);

   RARCH_LOG("device name: %s\n", buf);

   result = true;

error:
   (*env)->DeleteLocalRef(env, name);
   (*env)->DeleteLocalRef(env, class);
   (*env)->DeleteLocalRef(env, device);

   return result;
}

static void dump_locals() {
	JNIEnv     *env        = (JNIEnv*)jni_thread_getenv();
	jclass vm_class = (*env)->FindClass(env, "dalvik/system/VMDebug");
	jmethodID dump_mid = (*env)->GetStaticMethodID(env, vm_class, "dumpReferenceTables", "()V" );
	(*env)->CallStaticVoidMethod(env, vm_class, dump_mid );
}

static bool android_input_lookup_name(char *buf,
      int *vendorId, int *productId, size_t size, int id)
{
   jmethodID getVendorId  = NULL;
   jmethodID getProductId = NULL;
   jmethodID getName      = NULL;
   jobject device         = NULL;
   jobject name           = NULL;
   jmethodID method       = NULL;
   jclass class           = NULL;
   const char *str        = NULL;
   JNIEnv     *env        = (JNIEnv*)jni_thread_getenv();
   bool result = false;

   if (!env)
      goto error;

   RARCH_LOG("Using new lookup");

   FIND_CLASS(env, class, "android/view/InputDevice");
   if (!class)
      goto error;

   GET_STATIC_METHOD_ID(env, method, class, "getDevice",
         "(I)Landroid/view/InputDevice;");
   if (!method)
      goto error;

   CALL_OBJ_STATIC_METHOD_PARAM(env, device, class, method, (jint)id);
   if (!device)
   {
      RARCH_ERR("Failed to find device for ID: %d\n", id);
      goto error;
   }

   GET_METHOD_ID(env, getName, class, "getName", "()Ljava/lang/String;");
   if (!getName)
      goto error;

   CALL_OBJ_METHOD(env, name, device, getName);
   if (!name)
   {
      RARCH_ERR("Failed to find name for device ID: %d\n", id);
      goto error;
   }

   buf[0] = '\0';

   str = (*env)->GetStringUTFChars(env, name, 0);
   if (str)
      strlcpy(buf, str, size);
   (*env)->ReleaseStringUTFChars(env, name, str);

   RARCH_LOG("device name: %s\n", buf);

   GET_METHOD_ID(env, getVendorId, class, "getVendorId", "()I");
   if (!getVendorId)
      goto error;

   CALL_INT_METHOD(env, *vendorId, device, getVendorId);

   RARCH_LOG("device vendor id: %d\n", *vendorId);

   GET_METHOD_ID(env, getProductId, class, "getProductId", "()I");
   if (!getProductId)
      goto error;

   *productId = 0;
   CALL_INT_METHOD(env, *productId, device, getProductId);

   RARCH_LOG("device product id: %d\n", *productId);

   result = true;

error:
   (*env)->DeleteLocalRef(env, name);
   (*env)->DeleteLocalRef(env, class);
   (*env)->DeleteLocalRef(env, device);

   return result;
}


static bool android_input_get_descriptor(char *buf, size_t size, int id)
{
   jobject descriptor      = NULL;
   jmethodID getDescriptor = NULL;
   jobject device    = NULL;
   jmethodID method  = NULL;
   jclass    class   = 0;
   const char *str   = NULL;
   JNIEnv     *env   = (JNIEnv*)jni_thread_getenv();
   bool result = false;

   if (!env)
      goto error;

   FIND_CLASS(env, class, "android/view/InputDevice");
   if (!class)
      goto error;

   GET_STATIC_METHOD_ID(env, method, class, "getDevice",
         "(I)Landroid/view/InputDevice;");
   if (!method)
      goto error;

   CALL_OBJ_STATIC_METHOD_PARAM(env, device, class, method, (jint)id);
   if (!device)
   {
      RARCH_ERR("Failed to find device for ID: %d\n", id);
      goto error;
   }

   GET_METHOD_ID(env, getDescriptor, class, "getDescriptor", "()Ljava/lang/String;");
   if (!getDescriptor)
      goto error;

   CALL_OBJ_METHOD(env, descriptor, device, getDescriptor);
   if (!descriptor)
   {
      RARCH_ERR("Failed to find descriptor for device ID: %d\n", id);
      goto error;
   }

   buf[0] = '\0';

   str = (*env)->GetStringUTFChars(env, descriptor, 0);
   if (str)
      strlcpy(buf, str, size);
   (*env)->ReleaseStringUTFChars(env, descriptor, str);

   RARCH_LOG("device descriptor: %s\n", buf);

   result = true;

error:
   (*env)->DeleteLocalRef(env, descriptor);
   (*env)->DeleteLocalRef(env, device);
   (*env)->DeleteLocalRef(env, class);

   return result;
}

static jclass android_find_non_native_class(JNIEnv *env, char *classname) {
	struct android_app *android_app = (struct android_app*)g_android;

	jobject nativeActivity = android_app->activity->clazz;
	jclass acl = (*env)->GetObjectClass(env, nativeActivity);
	jmethodID getClassLoader = (*env)->GetMethodID(env, acl, "getClassLoader", "()Ljava/lang/ClassLoader;");

	jobject cls = (*env)->CallObjectMethod(env, nativeActivity, getClassLoader);
	jclass classLoader = (*env)->FindClass(env, "java/lang/ClassLoader");
	jmethodID findClass = (*env)->GetMethodID(env, classLoader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

	jstring strClassName = (*env)->NewStringUTF(env, classname);
	jclass _class = (jclass)((*env)->CallObjectMethod(env, cls, findClass, strClassName));

	(*env)->DeleteLocalRef(env, strClassName);
	(*env)->DeleteLocalRef(env, classLoader);
	return _class;
}

static void android_toast(char *message)
{
   jmethodID method  = NULL;
   jclass    class   = 0;
   jstring jmessage  = NULL;
   JNIEnv     *env   = (JNIEnv*)jni_thread_getenv();

   if (!env)
      goto error;

   class = android_find_non_native_class(env, "com.retroarch.browser.retroactivity.RetroBoxWrapper");
   if (!class)
      goto error;

   GET_STATIC_METHOD_ID(env, method, class, "toast",
         "(Ljava/lang/String;)V");
   if (!method)
      goto error;

   jmessage = (*env)->NewStringUTF(env, message);

   CALL_VOID_STATIC_METHOD_PARAM(env, class, method, jmessage);

error:
   (*env)->DeleteLocalRef(env, jmessage);
   (*env)->DeleteLocalRef(env, class);
}


static void engine_handle_cmd(void)
{
   int8_t cmd;
   struct android_app *android_app = (struct android_app*)g_android;
   runloop_t *runloop = rarch_main_get_ptr();
   driver_t  *driver  = driver_get_ptr();
   rarch_system_info_t *system = rarch_system_info_get_ptr();

   if (read(android_app->msgread, &cmd, sizeof(cmd)) != sizeof(cmd))
      cmd = -1;

   switch (cmd)
   {
      case APP_CMD_INPUT_CHANGED:
         slock_lock(android_app->mutex);

         if (android_app->inputQueue)
            AInputQueue_detachLooper(android_app->inputQueue);

         android_app->inputQueue = android_app->pendingInputQueue;

         if (android_app->inputQueue)
         {
            RARCH_LOG("Attaching input queue to looper");
            AInputQueue_attachLooper(android_app->inputQueue,
                  android_app->looper, LOOPER_ID_INPUT, NULL,
                  NULL);
         }

         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);

         break;

      case APP_CMD_INIT_WINDOW:
         slock_lock(android_app->mutex);
         android_app->window = android_app->pendingWindow;
         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);

         if (runloop->is_paused)
            event_command(EVENT_CMD_REINIT);
         break;

      case APP_CMD_RESUME:
    	  if (android_app->keep_pause) {
    		  // this is a delayed unpause. Copied from APP_CMD_GAINED_FOCUS
    		  // we should not unpause RetroArch if last action was open the context menu
			 runloop->is_paused = false;
			 runloop->is_idle   = false;

			 if ((android_app->sensor_state_mask
					  & (UINT64_C(1) << RETRO_SENSOR_ACCELEROMETER_ENABLE))
				   && android_app->accelerometerSensor == NULL
				   && driver->input_data)
				android_input_set_sensor_state(driver->input_data, 0,
					  RETRO_SENSOR_ACCELEROMETER_ENABLE,
					  android_app->accelerometer_event_rate);

			 android_app->keep_pause = false;
    	  }
         slock_lock(android_app->mutex);
         android_app->activityState = cmd;
         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);
         break;

      case APP_CMD_START:
         slock_lock(android_app->mutex);
         android_app->activityState = cmd;
         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);
         break;

      case APP_CMD_PAUSE:
         slock_lock(android_app->mutex);
         android_app->activityState = cmd;
         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);

         if (!system->shutdown)
         {
            RARCH_LOG("Pausing RetroArch.\n");
            runloop->is_paused = true;
            runloop->is_idle   = true;
         }
         break;

      case APP_CMD_STOP:
         slock_lock(android_app->mutex);
         android_app->activityState = cmd;
         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);
         break;

      case APP_CMD_CONFIG_CHANGED:
         break;
      case APP_CMD_TERM_WINDOW:
         slock_lock(android_app->mutex);

         /* The window is being hidden or closed, clean it up. */
         /* terminate display/EGL context here */

#if 0
         RARCH_WARN("Window is terminated outside PAUSED state.\n");
#endif

         android_app->window = NULL;
         scond_broadcast(android_app->cond);
         slock_unlock(android_app->mutex);
         break;

      case APP_CMD_GAINED_FOCUS:
    	  if (!android_app->keep_pause) {
			 runloop->is_paused = false;
			 runloop->is_idle   = false;

			 if ((android_app->sensor_state_mask
					  & (UINT64_C(1) << RETRO_SENSOR_ACCELEROMETER_ENABLE))
				   && android_app->accelerometerSensor == NULL
				   && driver->input_data)
				android_input_set_sensor_state(driver->input_data, 0,
					  RETRO_SENSOR_ACCELEROMETER_ENABLE,
					  android_app->accelerometer_event_rate);
    	 }
         break;
      case APP_CMD_LOST_FOCUS:
         /* Avoid draining battery while app is not being used. */
         if ((android_app->sensor_state_mask
                  & (UINT64_C(1) << RETRO_SENSOR_ACCELEROMETER_ENABLE))
               && android_app->accelerometerSensor != NULL
               && driver->input_data)
            android_input_set_sensor_state(driver->input_data, 0,
                  RETRO_SENSOR_ACCELEROMETER_DISABLE,
                  android_app->accelerometer_event_rate);
         break;

      case APP_CMD_DESTROY:
         system->shutdown = true;
         break;
   }
}


const struct rarch_key_map rarch_key_map_android[] = {
   { AKEYCODE_0, RETROK_0 },
   { AKEYCODE_1, RETROK_1 },
   { AKEYCODE_2, RETROK_2 },
   { AKEYCODE_3, RETROK_3 },
   { AKEYCODE_4, RETROK_4 },
   { AKEYCODE_5, RETROK_5 },
   { AKEYCODE_6, RETROK_6 },
   { AKEYCODE_7, RETROK_7 },
   { AKEYCODE_8, RETROK_8 },
   { AKEYCODE_9, RETROK_9 },
   { AKEYCODE_A, RETROK_a },
   { AKEYCODE_B, RETROK_b },
   { AKEYCODE_C, RETROK_c },
   { AKEYCODE_D, RETROK_d },
   { AKEYCODE_E, RETROK_e },
   { AKEYCODE_F, RETROK_f },
   { AKEYCODE_G, RETROK_g },
   { AKEYCODE_H, RETROK_h },
   { 0, RETROK_UNKNOWN },
};

static void *android_input_init(void)
{
   int32_t sdk;
   settings_t *settings = config_get_ptr();
   android_input_t *android = (android_input_t*)
      calloc(1, sizeof(*android));

   if (!android)
      return NULL;

   android->is_back_pressed = false;
   android->pads_connected = 0;
   android->joypad         = input_joypad_init_driver(
         settings->input.joypad_driver, android);

   // load known retroboxtv gamepads
   for(int i=0; i<MAX_USERS; i++) {
	   if (strlen(settings->input.device_descriptor[i])>0) {
		   int port = android->pads_connected;
		   android->pad_states[port].id = -1;
		   android->pad_states[port].port = -1;
		   android->pad_states[port].ignore_back = true;
		   strcpy(android->pad_states[port].name, "unknown");
		   strlcpy(android->pad_states[port].descriptor, settings->input.device_descriptor[i],
		         sizeof(android->pad_states[port].descriptor));

		   android->pads_connected++;
		   RARCH_LOG("Added device descriptor %s for gamepad %d", android->pad_states[port].descriptor, android->pads_connected);
	   }
   }

   frontend_android_get_version_sdk(&sdk);

   RARCH_LOG("sdk version: %d\n", sdk);

   if (sdk >= 19)
      engine_lookup_name = android_input_lookup_name;
   else
      engine_lookup_name = android_input_lookup_name_prekitkat;

   input_keymaps_init_keyboard_lut(rarch_key_map_android);

   return android;
}

static int zeus_id = -1;
static int zeus_second_id = -1;

static INLINE int android_input_poll_event_type_motion(
      android_input_t *android, AInputEvent *event,
      int port, int source)
{
   int getaction, action;
   size_t motion_ptr;
   bool keyup;

   //RARCH_LOG("motion check source %x against %x | %x", source, AINPUT_SOURCE_TOUCHSCREEN, AINPUT_SOURCE_MOUSE);

   if (source & ~(AINPUT_SOURCE_TOUCHSCREEN | AINPUT_SOURCE_MOUSE))
      return 1;

   getaction  = AMotionEvent_getAction(event);
   action     = getaction & AMOTION_EVENT_ACTION_MASK;
   motion_ptr = getaction >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
   keyup      = (
         action == AMOTION_EVENT_ACTION_UP ||
         action == AMOTION_EVENT_ACTION_CANCEL ||
         action == AMOTION_EVENT_ACTION_POINTER_UP) ||
      (source == AINPUT_SOURCE_MOUSE &&
       action != AMOTION_EVENT_ACTION_DOWN &&
       action != AMOTION_EVENT_ACTION_MOVE &&
       action != AMOTION_EVENT_ACTION_HOVER_MOVE);

   if (action == AMOTION_EVENT_ACTION_HOVER_ENTER || action == AMOTION_EVENT_ACTION_HOVER_EXIT) {
	   // ignore these events
	   return 0;
   }

   android->motion_from_hover = action == AMOTION_EVENT_ACTION_HOVER_MOVE;

   // RARCH_LOG("motion keyup is %s action is %d", keyup?"true":"false", action);

   if (keyup && motion_ptr < MAX_TOUCH)
   {

	   // RARCH_LOG("motion keyup on motion_ptr = %d", motion_ptr);
  	 if (motion_ptr == 0) {
  		gettimeofday(&android->mouse_button_click_stop, NULL);
  		suseconds_t delta_msec =
  				(android->mouse_button_click_stop.tv_sec  * 1000 + android->mouse_button_click_stop.tv_usec  / 1000) -
  				(android->mouse_button_click_start.tv_sec * 1000 + android->mouse_button_click_start.tv_usec / 1000);
 		int mouse_button = 0;
  		if (delta_msec < 300) {
  			mouse_button = 1;
  		} else if (delta_msec < 1000) {
  			mouse_button = 2;
		} else if (delta_msec < 3000) {
			mouse_button = 3;
		}
  		RARCH_LOG("click flag enabled on motion_ptr %i button %i delta msec:%lu", motion_ptr, mouse_button, delta_msec);
  		android->mouse_button_click = mouse_button;
  	 }

      memmove(android->pointer + motion_ptr,
            android->pointer + motion_ptr + 1,
            (MAX_TOUCH - motion_ptr - 1) * sizeof(struct input_pointer));
      if (android->pointer_count > 0)
         android->pointer_count--;
   }
   else
   {
      float x, y;
      int pointer_max = min(AMotionEvent_getPointerCount(event), MAX_TOUCH);
      // RARCH_LOG("motion pointer_max is %d", pointer_max);

      for (motion_ptr = 0; motion_ptr < pointer_max; motion_ptr++)
      {
         x = AMotionEvent_getX(event, motion_ptr);
         y = AMotionEvent_getY(event, motion_ptr);

         // RARCH_LOG("motion x, y = %f, %f", x, y);

         input_translate_coord_viewport(x, y,
               &android->pointer[motion_ptr].x,
               &android->pointer[motion_ptr].y,
               &android->pointer[motion_ptr].full_x,
               &android->pointer[motion_ptr].full_y);

		 if (motion_ptr == 0 && (action == AMOTION_EVENT_ACTION_DOWN || action == AMOTION_EVENT_ACTION_HOVER_EXIT)) {
			 gettimeofday(&android->mouse_button_click_start, NULL);
		 }

         android->pointer_count = max(
               android->pointer_count,
               motion_ptr + 1);

         // RARCH_LOG("motion pointer_count = %d", android->pointer_count);
      }
   }

   return 0;
}

static INLINE void android_input_poll_event_type_key(
      android_input_t *android, struct android_app *android_app,
      AInputEvent *event, int port, int keycode, int source,
      int type_event, int *handled)
{
   uint8_t *buf = port >= 0 ? android->pad_state[port] : NULL;
   int action  = AKeyEvent_getAction(event);

   /* some controllers send both the up and down events at once
    * when the button is released for "special" buttons, like menu buttons
    * work around that by only using down events for meta keys (which get
    * cleared every poll anyway)
    */

   if (port >= 0) {
	   switch (action)
	   {
		  case AKEY_EVENT_ACTION_UP:
			 BIT_CLEAR(buf, keycode);
			 break;
		  case AKEY_EVENT_ACTION_DOWN:
			 BIT_SET(buf, keycode);
			 break;
	   }
   }

   if (keycode == AKEYCODE_BACK && (port < 0 || !android->pad_states[port].ignore_back)) {
	   android->is_back_pressed = action == AKEY_EVENT_ACTION_DOWN;
   }

   if (keycode == AKEYCODE_SEARCH && (port >=0 && android->pad_states[port].is_nvidia)) {
	   if (action == AKEY_EVENT_ACTION_DOWN)
		   android->is_back_pressed = true;
   }

   if (port == 0) {
	   if (keycode == android->joypad_0_select_keycode) {
		   android->joypad_0_select_pressed = action == AKEY_EVENT_ACTION_DOWN;
	   }
	   if (keycode == android->joypad_0_r3_keycode) {
		   android->joypad_0_r3_pressed = action == AKEY_EVENT_ACTION_DOWN;
	   }
   }

   if ((keycode == AKEYCODE_VOLUME_UP || keycode == AKEYCODE_VOLUME_DOWN))
      *handled = 0;
}

static int android_input_get_id_port(android_input_t *android, int id,
      int source)
{
   unsigned i;
   if (source & (AINPUT_SOURCE_TOUCHSCREEN | AINPUT_SOURCE_MOUSE |
            AINPUT_SOURCE_TOUCHPAD))
      return 0; /* touch overlay is always user 1 */

   for (i = 0; i < android->pads_connected; i++)
      if (android->pad_states[i].id>=0 && android->pad_states[i].id == id)
         return i;

   RARCH_LOG("Port not found for device id %d", id);
   return -1;
}



/* Returns the index inside android->pad_state */
static int android_input_get_id_index_from_name(android_input_t *android,
      const char *name)
{
   int i;
   for (i = 0; i < android->pads_connected; i++)
   {
      if (!strcmp(name, android->pad_states[i].name))
         return i;
   }

   return -1;
}

static int android_is_gamepad_button(int keycode) {

	static int buttons[] = {
			RETRO_DEVICE_ID_JOYPAD_A,  RETRO_DEVICE_ID_JOYPAD_B,  RETRO_DEVICE_ID_JOYPAD_X,      RETRO_DEVICE_ID_JOYPAD_Y,
			RETRO_DEVICE_ID_JOYPAD_L,  RETRO_DEVICE_ID_JOYPAD_R,  RETRO_DEVICE_ID_JOYPAD_L2,     RETRO_DEVICE_ID_JOYPAD_R2,
			RETRO_DEVICE_ID_JOYPAD_L3, RETRO_DEVICE_ID_JOYPAD_R3, RETRO_DEVICE_ID_JOYPAD_SELECT, RETRO_DEVICE_ID_JOYPAD_START, -1};

	for(int i=0; buttons[i]>=0; i++) {
		if (buttons[i] == keycode) return true;
	}
	return false;
}

static void handle_hotplug(android_input_t *android,
      struct android_app *android_app, unsigned id,
      int source, int keycode, int *detected_port)
{
   char device_name[256]        = {0};
   char name_buf[256]           = {0};
   char device_descriptor[256]  = {0};
   unsigned port                = android->pads_connected;
   int vendorId                 = 0;
   int productId                = 0;
   bool back_mapped             = false;
   settings_t         *settings = config_get_ptr();
   unsigned bind;

   if (android->pads_connected > MAX_PADS)
   {
      RARCH_ERR("Max number of pads reached.\n");
      return;
   }

   if (!engine_lookup_name(device_name, &vendorId, &productId, sizeof(device_name), id))
   {
      RARCH_ERR("Could not look up device name or IDs.\n");
      return;
   }

   if (!android_input_get_descriptor(device_descriptor, sizeof(device_descriptor), id))
   {
	  RARCH_ERR("Could not look up device descriptor\n");
	  return;
   }

   if (0) {

	   /* FIXME: Ugly hack, see other FIXME note below. */
	   if (strstr(device_name, "keypad-game-zeus") ||
			 strstr(device_name, "keypad-zeus"))
	   {
		  if (zeus_id < 0)
		  {
			 RARCH_LOG("zeus_pad 1 detected: %u\n", id);
			 zeus_id = id;
		  }
		  else
		  {
			 RARCH_LOG("zeus_pad 2 detected: %u\n", id);
			 zeus_second_id = id;
		  }
		  strlcpy(name_buf, device_name, sizeof(name_buf));
	   }
	   /* followed by a 4 (hex) char HW id */
	   else if (strstr(device_name, "iControlPad-"))
		  strlcpy(name_buf, "iControlPad HID Joystick profile", sizeof(name_buf));
	   else if (strstr(device_name, "TTT THT Arcade console 2P USB Play"))
	   {
		  //FIXME - need to do a similar thing here as we did for nVidia Shield
		  //and Xperia Play. We need to keep 'count' of the amount of similar (grouped)
		  //devices.
		  //
		  //For Xperia Play - count similar devices and bind them to the same 'user'
		  //port
		  //
		  //For nVidia Shield - see above
		  //
		  //For TTT HT - keep track of how many of these 'pads' are already
		  //connected, and based on that, assign one of them to be User 1 and
		  //the other to be User 2.
		  //
		  //If this is finally implemented right, then these port conditionals can go.
		  if (port == 0)
			 strlcpy(name_buf, "TTT THT Arcade (User 1)", sizeof(name_buf));
		  else if (port == 1)
			 strlcpy(name_buf, "TTT THT Arcade (User 2)", sizeof(name_buf));
	   }
	   else if (strstr(device_name, "360 Wireless"))
		  strlcpy(name_buf, "XBox 360 Wireless", sizeof(name_buf));
	   else if (strstr(device_name, "Microsoft"))
	   {
		  if (strstr(device_name, "Dual Strike"))
			 strlcpy(device_name, "SideWinder Dual Strike", sizeof(device_name));
		  else if (strstr(device_name, "SideWinder"))
			 strlcpy(name_buf, "SideWinder Classic", sizeof(name_buf));
	   }
	   else if (strstr(device_name, "NVIDIA Corporation NVIDIA Controller v01.01"))
	   {
		  /* Built-in shield contrlleris always user 1. FIXME: This is kinda ugly.
		   * We really need to find a way to detect useless input devices
		   * like gpio-keys in a general way.
		   */
		  port = 0;
		  strlcpy(name_buf, device_name, sizeof(name_buf));
	   }
	   else if (strstr(device_name, "Virtual") ||
			 (strstr(device_name, "gpio") && strstr(android->pad_states[0].name,"NVIDIA Corporation NVIDIA Controller v01.01")))
	   {
		  /* If built-in shield controller is detected bind the virtual and gpio devices to the same port*/
		  port = 0;
		  strlcpy(name_buf, "NVIDIA Corporation NVIDIA Controller v01.01", sizeof(name_buf));
	   }
	   else if (
			 strstr(device_name, "PLAYSTATION(R)3") ||
			 strstr(device_name, "Dualshock3") ||
			 strstr(device_name, "Sixaxis")
			 )
		  strlcpy(name_buf, "PlayStation3", sizeof(name_buf));
	   else if (strstr(device_name, "MOGA"))
		  strlcpy(name_buf, "Moga IME", sizeof(name_buf));
	   else if (device_name[0] != '\0')
		  strlcpy(name_buf, device_name, sizeof(name_buf));

	   if (strstr(android_app->current_ime, "net.obsidianx.android.mogaime"))
		  strlcpy(name_buf, android_app->current_ime, sizeof(name_buf));
	   else if (strstr(android_app->current_ime, "com.ccpcreations.android.WiiUseAndroid"))
		  strlcpy(name_buf, android_app->current_ime, sizeof(name_buf));
	   else if (strstr(android_app->current_ime, "com.hexad.bluezime"))
		  strlcpy(name_buf, android_app->current_ime, sizeof(name_buf));

   } else {
	   if ((strstr(device_name, "Virtual") || strstr(device_name, "gpio")) &&
			   strstr(android->pad_states[0].name,"NVIDIA Corporation NVIDIA Controller")) {
		  /* If built-in shield controller is detected bind the virtual and gpio devices to the same port*/
		  port = 0;
		  strlcpy(name_buf, android->pad_states[0].name, sizeof(name_buf));
	   } else if (strstr(device_name, "Virtual")) {
		   return;
	   }
	   strlcpy(name_buf, device_name, sizeof(name_buf));
   }

   if (name_buf[0] != '\0')
	  strlcpy(settings->input.device_names[port],
			name_buf, sizeof(settings->input.device_names[port]));

   if (settings->input.autodetect_enable)
   {
      unsigned      autoconfigured = false;
      autoconfig_params_t params   = {{0}};

      strlcpy(params.name, name_buf, sizeof(params.name));
      strlcpy(params.display_name, name_buf, sizeof(params.display_name));

      for(int i=0; i<strlen(params.name); i++) {
    	  params.name[i] = tolower(params.name[i]);
	  }

      RARCH_LOG("Port %d: %s.\n", port, params.display_name);

      params.idx = port;
      params.vid = vendorId;
      params.pid = productId;
      strlcpy(params.driver, android_joypad.ident, sizeof(params.driver));
      autoconfigured = input_config_autoconfigure_joypad(&params);

      if (autoconfigured) {
         if (settings->input.autoconf_binds[port][RARCH_MENU_TOGGLE].joykey != 0)
            back_mapped = true;
      } else {
    	  return;
      }

      int translated_code = -1;
      for(bind = 0; bind < RARCH_BIND_LIST_END; bind++) {
    	  int joykey = settings->input.autoconf_binds[port][bind].joykey;
    	  if (joykey == keycode) {
    		  translated_code = bind;
    		  break;
    	  }
      }

      if (!android_is_gamepad_button(translated_code)) return;

   }

   /*
   int preset_device = -1;
   for(int i=0; i<android->pads_connected; i++) {
	   if (!strcmp(android->pad_states[i].descriptor, device_descriptor)
			&& (android->pad_states[i].id<0 || android->pad_states[i].id == id)) {
		   preset_device = i;
		   break;
	   }
   }

   if (preset_device<0) {
	   RARCH_LOG("Looking for known device descriptor %s. NOT FOUND", device_descriptor);
   } else {
	   RARCH_LOG("Looking for known device descriptor %s. Found at gamepad %d", device_descriptor, preset_device+1);
   }
   port = preset_device>=0 ? preset_device : android->pads_connected;
   */

   *detected_port = port;

   bool ignore_back = false;
   for(bind = 0; !ignore_back && bind < RARCH_BIND_LIST_END; bind++) {
	   int joykey = settings->input.autoconf_binds[port][bind].joykey;
	   ignore_back = joykey == AKEYCODE_BACK;
   }

   if (port == 0) {
	   android->joypad_0_select_keycode = settings->input.autoconf_binds[0][RETRO_DEVICE_ID_JOYPAD_SELECT].joykey;
	   android->joypad_0_r3_keycode     = settings->input.autoconf_binds[0][RETRO_DEVICE_ID_JOYPAD_R3].joykey;
	   android->joypad_0_select_pressed = false;
	   android->joypad_0_r3_pressed     = false;
   }

   if (!ignore_back && !back_mapped && settings->input.back_as_menu_toggle_enable) {
      settings->input.autoconf_binds[port][RARCH_MENU_TOGGLE].joykey = AKEYCODE_BACK;
   }

   android->pad_states[port].id = id;
   android->pad_states[port].port = port;
   android->pad_states[port].ignore_back = ignore_back;
   android->pad_states[port].is_nvidia = strstr(device_name, "Virtual") != NULL || strcasestr(device_name, "NVIDIA") != NULL;
   strlcpy(android->pad_states[port].name, name_buf,
         sizeof(android->pad_states[port].name));

   strlcpy(android->pad_states[port].descriptor, device_descriptor, sizeof(android->pad_states[port].descriptor));
   android->pads_connected++;

   RARCH_LOG("Current %d devices", android->pads_connected);
   for(int i=0; i<android->pads_connected; i++) {
	   RARCH_LOG("Current gamepad %d: %s (%s)", i+1, android->pad_states[i].name, android->pad_states[i].descriptor);
   }

   if (settings->input_display_hotplug) {
	   char msg[1000];
	   sprintf(msg, "%s is Player %d", device_name, port+1);
	   android_toast(msg);
   }

}

static int android_input_get_id(android_input_t *android, AInputEvent *event)
{
   int id = AInputEvent_getDeviceId(event);

   /* Needs to be cleaned up */
   if (id == zeus_second_id)
      id = zeus_id;

   settings_t *settings = config_get_ptr();
   if (settings->input.join_device_ids && id == 2) {
	   id = 1;
   }

   return id;
}

static void android_input_handle_input(void *data)
{
   AInputEvent *event = NULL;
   android_input_t    *android     = (android_input_t*)data;
   struct android_app *android_app = (struct android_app*)g_android;

   /* Read all pending events. */
   while (AInputQueue_hasEvents(android_app->inputQueue))
   {
      while (AInputQueue_getEvent(android_app->inputQueue, &event) >= 0)
      {
         int32_t   handled = 1;
         int predispatched = AInputQueue_preDispatchEvent(android_app->inputQueue, event);
         int        source = AInputEvent_getSource(event);
         int    type_event = AInputEvent_getType(event);
         int            id = android_input_get_id(android, event);
         int          port = android_input_get_id_port(android, id, source);

         if (port < 0) {
        	// do hotplug only with gamepad buttons
        	RARCH_LOG("Evaluate hotplug: start\n");
        	if (type_event == AINPUT_EVENT_TYPE_KEY) {
        		RARCH_LOG("Evaluate hotplug: event is key\n");
        		int keycode = AKeyEvent_getKeyCode(event);
        		RARCH_LOG("Evaluate hotplug: keycode is %d\n", keycode);
				handle_hotplug(android, android_app, id, source, keycode, &port);
        	}
         }

         switch (type_event)
         {
            case AINPUT_EVENT_TYPE_MOTION:
               if (android_input_poll_event_type_motion(android, event,
                        port, source))
                  engine_handle_dpad(android, event, port, source);
               break;
            case AINPUT_EVENT_TYPE_KEY:
               {
                  int keycode = AKeyEvent_getKeyCode(event);
                  android_input_poll_event_type_key(android, android_app,
                        event, port, keycode, source, type_event, &handled);
                  if (keycode == AKEYCODE_DPAD_LEFT || keycode == AKEYCODE_DPAD_RIGHT ||
                	  keycode == AKEYCODE_DPAD_UP   || keycode == AKEYCODE_DPAD_DOWN) {
                	  engine_handle_dpad_real(android, port);
                  }
               }
               break;
         }

         if (!predispatched)
            AInputQueue_finishEvent(android_app->inputQueue, event,
                  handled);
      }
   }
}

static void android_input_handle_user(void *data)
{
   android_input_t    *android     = (android_input_t*)data;
   struct android_app *android_app = (struct android_app*)g_android;

   if ((android_app->sensor_state_mask & (UINT64_C(1) <<
               RETRO_SENSOR_ACCELEROMETER_ENABLE))
         && android_app->accelerometerSensor)
   {
      ASensorEvent event;
      while (ASensorEventQueue_getEvents(android->sensorEventQueue, &event, 1) > 0)
      {
         android->accelerometer_state.x = event.acceleration.x;
         android->accelerometer_state.y = event.acceleration.y;
         android->accelerometer_state.z = event.acceleration.z;
      }
   }
}

/* Handle all events. If our activity is in pause state,
 * block until we're unpaused.
 */
static void android_input_poll(void *data)
{
   int ident;
   driver_t *driver                = driver_get_ptr();
   const input_driver_t *input     = driver ? (const input_driver_t*)driver->input : NULL;

   if (!input)
      return;

   while ((ident =
            ALooper_pollAll((input->key_pressed(driver->input_data, RARCH_PAUSE_TOGGLE))
               ? -1 : 1,
               NULL, NULL, NULL)) >= 0)
   {
      switch (ident)
      {
         case LOOPER_ID_INPUT:
            android_input_handle_input(data);
            break;
         case LOOPER_ID_USER:
            android_input_handle_user(data);
            break;
         case LOOPER_ID_MAIN:
            engine_handle_cmd();
            break;
      }
   }
}

bool android_run_events(void *data)
{
   rarch_system_info_t *system = rarch_system_info_get_ptr();
   int id = ALooper_pollOnce(-1, NULL, NULL, NULL);

   if (id == LOOPER_ID_MAIN)
      engine_handle_cmd();

   /* Check if we are exiting. */
   if (system->shutdown)
      return false;

   return true;
}

static int16_t android_input_state(void *data,
      const struct retro_keybind **binds, unsigned port, unsigned device,
      unsigned idx, unsigned id)
{
   android_input_t *android = (android_input_t*)data;


   if (idx != RETRO_DEVICE_INDEX_ANALOG_BUTTON) {
	   struct android_app *android_app = (struct android_app*)g_android;

	   if (android_app->is_mame_menu_request) {
			android_app->is_mame_menu_request = false;
			android->mame_trigger_state_l2 = true;
			android->mame_trigger_state_r2 = true;
	   }

	   if (android_app->is_mame_service_request) {
			android_app->is_mame_service_request = false;
			android->mame_trigger_state_l3 = true;
			android->mame_trigger_state_r3 = true;
	   }

	   if (device == RETRO_DEVICE_JOYPAD || device == RETRO_DEVICE_ANALOG) {
		   if (id == RETRO_DEVICE_ID_JOYPAD_L2 &&
				   (android->trigger_state[port][0] || android->mame_trigger_state_l2)) {
			   android->mame_trigger_state_l2 = false;
			   return true;
		   }
		   if (id == RETRO_DEVICE_ID_JOYPAD_R2 &&
				   (android->trigger_state[port][1] || android->mame_trigger_state_r2)) {
			   android->mame_trigger_state_r2 = false;
			   return true;
		   }
		   if (id == RETRO_DEVICE_ID_JOYPAD_L3 && (android->mame_trigger_state_l3)) {
			   android->mame_trigger_state_l2 = false;
			   return true;
		   }
		   if (id == RETRO_DEVICE_ID_JOYPAD_R2 && (android->mame_trigger_state_r3)) {
			   android->mame_trigger_state_r3 = false;
			   return true;
		   }
	   }
   }

   switch (device)
   {
      case RETRO_DEVICE_JOYPAD: {
    	  return input_joypad_pressed(android->joypad, port, binds[port], id);
      }

      case RETRO_DEVICE_ANALOG: {
    	  if (idx == RETRO_DEVICE_INDEX_ANALOG_BUTTON) {
    		  if (id == RETRO_DEVICE_ID_JOYPAD_L2) return android->analog_state[port][8];
    		  if (id == RETRO_DEVICE_ID_JOYPAD_R2) return android->analog_state[port][9];
    	  }

    	  return input_joypad_analog(android->joypad, port, idx, id, binds[port]);
      }
      case RETRO_DEVICE_POINTER:
         switch (id)
         {
            case RETRO_DEVICE_ID_POINTER_X:
               return android->pointer[idx].x;
            case RETRO_DEVICE_ID_POINTER_Y:
               return android->pointer[idx].y;
            case RETRO_DEVICE_ID_POINTER_PRESSED:
               return (idx < android->pointer_count) &&
                  (android->pointer[idx].x != -0x8000) &&
                  (android->pointer[idx].y != -0x8000);
            case RARCH_DEVICE_ID_POINTER_BACK:
               return BIT_GET(android->pad_state[0], AKEYCODE_BACK);
         }
         break;
      case RARCH_DEVICE_POINTER_SCREEN:
         switch (id)
         {
            case RETRO_DEVICE_ID_POINTER_X:
               return android->pointer[idx].full_x;
            case RETRO_DEVICE_ID_POINTER_Y:
               return android->pointer[idx].full_y;
            case RETRO_DEVICE_ID_POINTER_PRESSED:
               return (idx < android->pointer_count) &&
                  (android->pointer[idx].full_x != -0x8000) &&
                  (android->pointer[idx].full_y != -0x8000);
            case RARCH_DEVICE_ID_POINTER_BACK:
               return BIT_GET(android->pad_state[0], AKEYCODE_BACK);
         }
         break;
         case RETRO_DEVICE_MOUSE: {
        	 bool clicked = false;
        	 switch(id) {
        	 case RETRO_DEVICE_ID_MOUSE_LEFT:
        		 clicked = android->mouse_button_click == 1;
        		 break;
        	 case RETRO_DEVICE_ID_MOUSE_RIGHT:
        		 clicked = android->mouse_button_click == 2;
        		 break;
        	 case RETRO_DEVICE_ID_MOUSE_MIDDLE:
        		 clicked = android->mouse_button_click == 3;
        		 break;
        	 case RETRO_DEVICE_ID_MOUSE_HOVER:
        		 // don't process as clicked... dirty but simple
        		 return android->motion_from_hover;
        	 }
        	 if (clicked) android->mouse_button_click = 0;
        	 return clicked;
        }
        break;
        case RETRO_DEVICE_KEYBOARD:
       	   //RARCH_LOG("state check for input %d = %s", id, "nonx");
       	break;
   }

   return 0;
}

static bool android_input_key_pressed(void *data, int key)
{
   android_input_t *android = (android_input_t*)data;
   settings_t *settings     = config_get_ptr();

   /* it seems that this is only for gamepad buttons */

   return input_joypad_pressed(android->joypad,
            0, settings->input.binds[0], key);
}

static bool android_input_meta_key_pressed(void *data, int key)
{
	android_input_t *android = (android_input_t*)data;
	if (key == RARCH_MENU_TOGGLE) {
		bool isPressed = android->is_back_pressed;
		android->is_back_pressed = false;
		return isPressed;
	}

	// Doing this via config didn't work and I gave up finding why
	// Binding code for this version at least is a real mess
	// So... forgive this uglyness

	bool isRewind      = key == RARCH_REWIND;
	bool isFastForward = key == RARCH_FAST_FORWARD_HOLD_KEY;
	if (isRewind || isFastForward) {
		settings_t *settings = config_get_ptr();

		bool isL2 = android->trigger_state[0][0];
		bool isR2 = android->trigger_state[0][1];

		int combo_mode = settings->input.rewind_forward_combo;
		bool combo =
				(combo_mode == COMBO_REWIND_FORWARD_SIMPLE) ||
				(combo_mode == COMBO_REWIND_FORWARD_SELECT && android->joypad_0_select_pressed) ||
				(combo_mode == COMBO_REWIND_FORWARD_R3     && android->joypad_0_r3_pressed);

		if (combo) {
			if (isL2 && isRewind) return true;
			if (isR2 && isFastForward) return true;
		}
	}

	return false;
}

static void android_input_free_input(void *data)
{
   android_input_t *android = (android_input_t*)data;
   if (!android)
      return;

   if (android->sensorManager)
      ASensorManager_destroyEventQueue(android->sensorManager,
            android->sensorEventQueue);

   free(data);
}

static uint64_t android_input_get_capabilities(void *data)
{
   (void)data;

   return
      (1 << RETRO_DEVICE_JOYPAD)  |
      (1 << RETRO_DEVICE_POINTER) |
      (1 << RETRO_DEVICE_KEYBOARD) |
      (1 << RETRO_DEVICE_ANALOG);
}

static void android_input_enable_sensor_manager(void *data)
{
   android_input_t        *android = (android_input_t*)data;
   struct android_app *android_app = (struct android_app*)g_android;

   android->sensorManager = ASensorManager_getInstance();
   android_app->accelerometerSensor =
      ASensorManager_getDefaultSensor(android->sensorManager,
         ASENSOR_TYPE_ACCELEROMETER);
   android->sensorEventQueue =
      ASensorManager_createEventQueue(android->sensorManager,
         android_app->looper, LOOPER_ID_USER, NULL, NULL);
}

static bool android_input_set_sensor_state(void *data, unsigned port,
      enum retro_sensor_action action, unsigned event_rate)
{
   android_input_t        *android = (android_input_t*)data;
   struct android_app *android_app = (struct android_app*)g_android;

   if (event_rate == 0)
      event_rate = 60;

   switch (action)
   {
      case RETRO_SENSOR_ACCELEROMETER_ENABLE:
         if (!android_app->accelerometerSensor)
            android_input_enable_sensor_manager(android);

         if (android_app->accelerometerSensor)
            ASensorEventQueue_enableSensor(android->sensorEventQueue,
                  android_app->accelerometerSensor);

         /* Events per second (in microseconds). */
         if (android_app->accelerometerSensor)
            ASensorEventQueue_setEventRate(android->sensorEventQueue,
                  android_app->accelerometerSensor, (1000L / event_rate)
                  * 1000);

         BIT64_CLEAR(android_app->sensor_state_mask, RETRO_SENSOR_ACCELEROMETER_DISABLE);
         BIT64_SET(android_app->sensor_state_mask, RETRO_SENSOR_ACCELEROMETER_ENABLE);
         return true;

      case RETRO_SENSOR_ACCELEROMETER_DISABLE:
         if (android_app->accelerometerSensor)
            ASensorEventQueue_disableSensor(android->sensorEventQueue,
                  android_app->accelerometerSensor);

         BIT64_CLEAR(android_app->sensor_state_mask, RETRO_SENSOR_ACCELEROMETER_ENABLE);
         BIT64_SET(android_app->sensor_state_mask, RETRO_SENSOR_ACCELEROMETER_DISABLE);
         return true;
      default:
         return false;
   }

   return false;
}

static float android_input_get_sensor_input(void *data,
      unsigned port,unsigned id)
{
   android_input_t *android = (android_input_t*)data;

   switch (id)
   {
      case RETRO_SENSOR_ACCELEROMETER_X:
         return android->accelerometer_state.x;
      case RETRO_SENSOR_ACCELEROMETER_Y:
         return android->accelerometer_state.y;
      case RETRO_SENSOR_ACCELEROMETER_Z:
         return android->accelerometer_state.z;
   }

   return 0;
}

static const input_device_driver_t *android_input_get_joypad_driver(void *data)
{
   android_input_t *android = (android_input_t*)data;
   if (!android)
      return NULL;
   return android->joypad;
}

static bool android_input_keyboard_mapping_is_blocked(void *data)
{
   android_input_t *android = (android_input_t*)data;
   if (!android)
      return false;
   return android->blocked;
}

static void android_input_keyboard_mapping_set_block(void *data, bool value)
{
   android_input_t *android = (android_input_t*)data;
   if (!android)
      return;
   android->blocked = value;
}

static void android_input_grab_mouse(void *data, bool state)
{
   (void)data;
   (void)state;
}

static bool android_input_set_rumble(void *data, unsigned port,
      enum retro_rumble_effect effect, uint16_t strength)
{
   (void)data;
   (void)port;
   (void)effect;
   (void)strength;

   return false;
}

input_driver_t input_android = {
   android_input_init,
   android_input_poll,
   android_input_state,
   android_input_key_pressed,
   android_input_meta_key_pressed,
   android_input_free_input,
   android_input_set_sensor_state,
   android_input_get_sensor_input,
   android_input_get_capabilities,
   "android",

   android_input_grab_mouse,
   NULL,
   android_input_set_rumble,
   android_input_get_joypad_driver,
   android_input_keyboard_mapping_is_blocked,
   android_input_keyboard_mapping_set_block,
};
