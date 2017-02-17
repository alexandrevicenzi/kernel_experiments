#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#include <pulse/mainloop-signal.h>

#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

#define TITLE(s) printf(COLOR_BLUE "%s:\n" COLOR_RESET, s)
#define INDEX(i) printf(COLOR_YELLOW "  #%d\t" COLOR_RESET, i)

// print_server_info prints the server information.
// see: https://freedesktop.org/software/pulseaudio/doxygen/structpa__server__info.html
void print_server_info(pa_context *c, const pa_server_info *i, void *userdata)
{
    TITLE("Server Information");

    printf("  server name:          %s\n", i->server_name);
    printf("  server version:       %s\n", i->server_version);
    printf("  user name:            %s\n", i->user_name);
    printf("  host name:            %s\n", i->host_name);
    printf("  default sink:         %s\n", i->default_sink_name);
    printf("  default source:       %s\n", i->default_source_name);

    char* s;

    switch (i->sample_spec.format)
    {
        case PA_SAMPLE_U8:
            s = "Unsigned 8 Bit PCM";
            break;
        case PA_SAMPLE_ALAW:
            s = "8 Bit a-Law";
            break;
        case PA_SAMPLE_ULAW:
            s = "8 Bit mu-Law";
            break;
        case PA_SAMPLE_S16LE:
            s = "Signed 16 Bit PCM, little endian (PC)";
            break;
        case PA_SAMPLE_S16BE:
            s = "Signed 16 Bit PCM, big endian";
            break;
        case PA_SAMPLE_FLOAT32LE:
            s = "32 Bit IEEE floating point, little endian (PC), range -1.0 to 1.0";
            break;
        case PA_SAMPLE_FLOAT32BE:
            s = "32 Bit IEEE floating point, big endian, range -1.0 to 1.0";
            break;
        case PA_SAMPLE_S32LE:
            s = "Signed 32 Bit PCM, little endian (PC)";
            break;
        case PA_SAMPLE_S32BE:
            s = "Signed 32 Bit PCM, big endian";
            break;
        case PA_SAMPLE_S24LE:
            s = "Signed 24 Bit PCM packed, little endian (PC)";
            break;
        case PA_SAMPLE_S24BE:
            s = "Signed 24 Bit PCM packed, big endian";
            break;
        case PA_SAMPLE_S24_32LE:
            s = "Signed 24 Bit PCM in LSB of 32 Bit words, little endian (PC)";
            break;
        case PA_SAMPLE_S24_32BE:
            s = "Signed 24 Bit PCM in LSB of 32 Bit words, big endian";
            break;
        default:
            s = "Invalid";
            break;
    }

    printf("  default sample type:  %s %d Channels %d Hz\n", s, i->sample_spec.channels, i->sample_spec.rate);
}

// print_modules prints all loaded modules.
void print_modules(pa_context *c, const pa_module_info *i, int eol, void *userdata)
{
    if (eol != 0)
        return;

    if (i->index == 0)
        TITLE("Loaded Modules");

    INDEX(i->index);

    // TODO: read "pa_proplist" if available.
    printf("name: %s argument: %s\n", i->name, i->argument);
}

void print_clients(pa_context *c, const pa_client_info *i, int eol, void *userdata)
{
    if (eol != 0)
        return;

    if (i->index == 0)
        TITLE("Clients");

    INDEX(i->index);

    // TODO: read "pa_proplist" if available.
    printf("name: %s driver: %s owner module: #%d\n", i->name, i->driver, i->owner_module);
}

void state_changed(pa_context *c, void *userdata)
{
    if (pa_context_get_state(c) == PA_CONTEXT_READY)
    {
        // get the server info.
        if (pa_context_get_server_info(c, print_server_info, NULL) < 0)
            printf("pa_context_get_server_info failed\n");

        // get the loaded modules.
        if (pa_context_get_module_info_list(c, print_modules, NULL) < 0)
            printf("pa_context_get_module_info_list failed\n");

        // get all clients connect to PA.
        if (pa_context_get_client_info_list(c, print_clients, NULL) < 0)
            printf("pa_context_get_client_info_list failed\n");
    }
}

void sigterm_callback(pa_mainloop_api*m, pa_signal_event *e, int sig, void *userdata)
{
    exit(0);
}

int main(int argc, char const *argv[])
{
    printf("Pulse Example\n");
    printf("Press Ctrl+C to quit.\n\n");

    int ret;

    struct pa_mainloop*     loop = pa_mainloop_new();
    struct pa_mainloop_api* api  = pa_mainloop_get_api(loop);

    if (pa_signal_init(api) < 0)
    {
        printf("pa_signal_init failed\n");
    }

    // hit Ctrl+C to exit this cli.
    struct pa_signal_event* pa_sigterm = pa_signal_new(SIGTERM, sigterm_callback, NULL);

    // create the connection context.
    // if this cli is running and you
    // open "Pulse Audio Manager" you will
    // see this client registered as "Pulse Example".
    struct pa_context* ctx = pa_context_new(api, "Pulse Example");

    pa_context_set_state_callback(ctx, state_changed, NULL);

    // connect to the Pulse Audio server.
    if (pa_context_connect(ctx, NULL, PA_CONTEXT_NOAUTOSPAWN, NULL) < 0)
    {
        printf("pa_context_connect failed\n");
        return 1;
    }

    // this will lock this thread.
    if (pa_mainloop_run(loop, &ret) < 0)
    {
        printf("pa_mainloop_run failed.\n");
    }

    pa_signal_free(pa_sigterm);
    pa_mainloop_free(loop);

    return ret;
}
