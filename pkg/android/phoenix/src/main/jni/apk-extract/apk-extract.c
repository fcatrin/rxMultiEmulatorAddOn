#include <file/file_extract.h>
#include <file/file_path.h>

#include <stdio.h>
#include <string.h>
#include <retro_miscellaneous.h>
#include "../native/com_retroarch_browser_NativeInterface.h"

//#define VERBOSE_LOG

#ifndef VERBOSE_LOG
#undef RARCH_LOG
#define RARCH_LOG(...)
#endif

struct userdata
{
   const char *subdir;
   const char *dest;
};

static int zlib_cb(const char *name, const char *valid_exts,
      const uint8_t *cdata,
      unsigned cmode, uint32_t csize, uint32_t size,
      uint32_t crc32, void *userdata)
{
   char path[PATH_MAX];
   char path_dir[PATH_MAX];
   struct userdata *user = userdata;
   const char *subdir = user->subdir;
   const char *dest   = user->dest;

   if (strstr(name, subdir) != name)
      return 1;

   name += strlen(subdir) + 1;

   fill_pathname_join(path, dest, name, sizeof(path));
   fill_pathname_basedir(path_dir, path, sizeof(path_dir));

   if (!path_mkdir(path_dir))
   {
      RARCH_ERR("Failed to create dir: %s.\n", path_dir);
      return 0;
   }

   RARCH_LOG("Extracting %s -> %s ...\n", name, path);

   if (!zlib_perform_mode(path, valid_exts,
            cdata, cmode, csize, size, crc32, userdata))
   {
      if (cmode == 0)
      {
         RARCH_ERR("Failed to write file: %s.\n", path);
         return 0;
      }
      goto error;
   }

   return 1;

error:
   RARCH_ERR("Failed to deflate to: %s.\n", path);
   return 0;
}

JNIEXPORT jboolean JNICALL Java_com_retroarch_browser_NativeInterface_extractArchiveTo(
      JNIEnv *env, jclass cls, jstring archive, jstring subdir, jstring dest)
{
   const char *archive_c = (*env)->GetStringUTFChars(env, archive, NULL);
   const char *subdir_c  = (*env)->GetStringUTFChars(env, subdir, NULL);
   const char *dest_c    = (*env)->GetStringUTFChars(env, dest, NULL);

   jboolean ret = JNI_TRUE;

   struct userdata data = {
      .subdir = subdir_c,
      .dest = dest_c,
   };

   if (!zlib_parse_file(archive_c, NULL, zlib_cb, &data))
   {
      RARCH_ERR("Failed to parse APK: %s.\n", archive_c);
      ret = JNI_FALSE;
   }

   (*env)->ReleaseStringUTFChars(env, archive, archive_c);
   (*env)->ReleaseStringUTFChars(env, subdir, subdir_c);
   (*env)->ReleaseStringUTFChars(env, dest, dest_c);
   return ret;
}
