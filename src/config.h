/**************************************************************************
* config :
* - parse config file in Panel struct.
*
* Check COPYING file for Copyright
*
**************************************************************************/

#ifndef CONFIG_H
#define CONFIG_H

#include <glib.h>

extern char *config_path;
extern char *snapshot_path;

void default_config();
// default global data

void cleanup_config();
// freed memory

gboolean config_read();

#endif
