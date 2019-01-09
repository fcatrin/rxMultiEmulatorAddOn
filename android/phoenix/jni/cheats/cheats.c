#include <stdlib.h>
#include <retro_miscellaneous.h>
#include <cheats.h>
#include "../native/com_retroarch_browser_NativeInterface.h"

cheat_manager_t *cheat_manager;

JNIEXPORT void JNICALL Java_com_retroarch_browser_NativeInterface_cheatsInit
  (JNIEnv *env, jclass _class, jstring jPath) {

	cheat_manager = cheat_manager_load("/sdcard/adventuresofbatmanrobinthe.cht");
	RARCH_LOG("cheat manager is null %s", cheat_manager == NULL ? "true" : "false");
	RARCH_LOG("cheats %d", cheat_manager->size);
}

JNIEXPORT jboolean JNICALL Java_com_retroarch_browser_NativeInterface_cheatsGetStatus
  (JNIEnv *env, jclass _class) {
	return false;
}

JNIEXPORT jobjectArray JNICALL Java_com_retroarch_browser_NativeInterface_cheatsGetNames
  (JNIEnv *env, jclass _class) {
	return NULL;
}

JNIEXPORT void JNICALL Java_com_retroarch_browser_NativeInterface_cheatsEnable
  (JNIEnv *env, jclass _class, jint index, jboolean enable) {

}
