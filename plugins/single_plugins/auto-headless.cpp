#include <wayfire/plugin.hpp>
#include <wayfire/output.hpp>
#include <wayfire/util.hpp>

extern "C" {
#include <wlr/backend/multi.h>
#include <wlr/backend/headless.h>
}

static void locate_headless_backend(wlr_backend *backend, void *data)
{
    if (wlr_backend_is_headless(backend))
    {
        wlr_backend **result = (wlr_backend**)data;
        *result = backend;
    }
}

class wayfire_auto_headless : public wf::plugin_interface_t
{
  private:
    static wlr_backend *get_headless_backend()
    {
        auto backend = wf::get_core().backend;
        wlr_backend *headless_backend = NULL;
        wlr_multi_for_each_backend(backend, locate_headless_backend,
            &headless_backend);
        return headless_backend;
    }

    void create_headless_output()
    {
      if (headless_output) {
        return;
      }

      LOGD("Creating headless output");

      auto headless_backend = get_headless_backend();
      if (!headless_backend) {
        return;
      }

      headless_output = wlr_headless_add_output(headless_backend, 1920, 1080);
    }

    void schedule_timer()
    {
      debouncer.disconnect();
      debouncer.set_timeout(200, [=]() {
        if (real_output_count > 0) {
          LOGD("Removing headless output");
          wlr_output_destroy(headless_output);
          headless_output = nullptr;
        } else {
          create_headless_output();
        }
        return false;
      });
    }

  public:
    void init() override
    {
      if (wlr_output_is_headless(output->handle)) {
        if (real_output_count == 0 && output->to_string() == "NOOP-1") {
          LOGD("First output is NOOP-1");
          schedule_timer();
        }
        return;
      }

      ++real_output_count;
      schedule_timer();
    }

    void fini() override
    {
      if (wlr_output_is_headless(output->handle)) {
        return;
      }

      --real_output_count;
      schedule_timer();
    }

  private:
    static int real_output_count;
    static wlr_output *headless_output;
    static wf::wl_timer debouncer;
};

int wayfire_auto_headless::real_output_count = 0;
wlr_output *wayfire_auto_headless::headless_output = nullptr;
wf::wl_timer wayfire_auto_headless::debouncer = {};

DECLARE_WAYFIRE_PLUGIN(wayfire_auto_headless);
