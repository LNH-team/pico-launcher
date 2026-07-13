#include "common.h"
#include "core/di.h"
#include "services/settings/JsonAppSettingsService.h"
#include "services/favorites/JsonFavoritesService.h"
#include "bgm/AudioStreamPlayer.h"
#include "bgm/IBgmService.h"
#include "bgm/BgmService.h"
#include "App.h"
#include "PicoLoaderProcess.h"
#include "settings/SettingsProcess.h"
#include "ProcessFactory.h"

namespace di = boost::di;

class injected_and_bound : public di::config
{
public:
    static auto policies(...) noexcept
    {
        using namespace di::policies;
        using namespace di::policies::operators;
        return di::make_policies(
            constructible(is_bound<di::_>{})
        );
    }
};

static auto diConfig = [] { return di::make_injector<injected_and_bound>(
    di::bind<RandomGenerator>().to((RandomGenerator&)*gRandomGenerator),
    di::bind<IAudioStreamPlayer>().to<AudioStreamPlayer>(),
    di::bind<IBgmService>().in(di::singleton).to<BgmService>(),
    di::bind<>().to((const char*)"/_pico/settings.json"),
    di::bind<IAppSettingsService>().in(di::singleton).to<JsonAppSettingsService>(),
    di::bind<IFavoritesService>().in(di::singleton).to<JsonFavoritesService>()
); };

#define REGISTER_PROCESS(name)  template <> std::unique_ptr<IProcess> ProcessFactory::Construct<name>() { return diConfig().create<std::unique_ptr<name>>(); }

REGISTER_PROCESS(App);
REGISTER_PROCESS(PicoLoaderProcess);
REGISTER_PROCESS(SettingsProcess);
