#include <stdlib.h>
#include <retro_miscellaneous.h>
#include <cheats.h>
#include "../../../frontend/drivers/com_retroarch_browser_retroactivity_RetroActivityFuture.h"

cheat_manager_t *cheat_manager;

JNIEXPORT void JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_cheatsInit
  (JNIEnv *env, jclass _class, jstring jPath) {

	if (cheat_manager != NULL) return;

	const char* path = (*env)->GetStringUTFChars(env, jPath, 0);

	cheat_manager = cheat_manager_load(path);
	(*env)->ReleaseStringUTFChars(env, jPath, path);

}

JNIEXPORT jbooleanArray JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_cheatsGetStatus
  (JNIEnv *env, jclass _class) {
	if (cheat_manager == NULL || cheat_manager->size == 0) return NULL;

	jbooleanArray result = (*env)->NewBooleanArray(env, cheat_manager->size);
	jboolean *elements = (*env)->GetBooleanArrayElements(env, result, NULL);

	for (int i = 0; i < cheat_manager->size; i++) {
		elements[i] = cheat_manager->cheats[i].state ? JNI_TRUE : JNI_FALSE;
	}

	(*env)->ReleaseBooleanArrayElements(env, result, elements, 0);

	return result;
}

JNIEXPORT jobjectArray JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_cheatsGetNames
  (JNIEnv *env, jclass _class) {
	if (cheat_manager == NULL) return NULL;
	return NULL;
}

JNIEXPORT void JNICALL Java_com_retroarch_browser_retroactivity_RetroActivityFuture_cheatsEnable
  (JNIEnv *env, jclass _class, jint index, jboolean enable) {

}
