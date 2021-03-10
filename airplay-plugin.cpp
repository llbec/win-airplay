#include <obs-module.h>
#include <strsafe.h>
#include <strmif.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("win-airplay", "en-US")

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Windows AirPlay source/encoder";
}

extern void RegisterAirPlaySource();

bool obs_module_load(void)
{
	RegisterAirPlaySource();

	/*obs_data_t *obs_settings = obs_data_create();
	obs_data_set_bool(obs_settings, "vcamEnabled", installed);
	obs_apply_private_data(obs_settings);
	obs_data_release(obs_settings);*/

	return true;
}
