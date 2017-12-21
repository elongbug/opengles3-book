
//
// Book:      OpenGL(R) ES 2.0 Programming Guide
// Authors:   Aaftab Munshi, Dan Ginsburg, Dave Shreiner
// ISBN-10:   0321502795
// ISBN-13:   9780321502797
// Publisher: Addison-Wesley Professional
// URLs:      http://safari.informit.com/9780321563835
//            http://www.opengles-book.com
//

// esUtil_Wayland.c
//
//    This file contains the LinuxX11 implementation of the windowing functions. 

///
// Includes
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/time.h>
#include "esUtil.h"

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

// Wayland Display related local variables
struct Display {
   struct wl_display *display;
   struct wl_registry *registry;
   struct wl_compositor *compositor;
   struct wl_shell *shell;
};

struct Window {
   struct Display *display;
   struct wl_egl_window *native;
   struct wl_surface *surface;
   struct wl_shell_surface *shell_surface;
};

static struct Display wl_disp = { 0 };
static struct Window wl_win = { 0 };


//////////////////////////////////////////////////////////////////
//
//  Private Functions and Member variables
//
//

static void registry_handle_global(void *data, struct wl_registry *registry,
                                   uint32_t name, const char *interface,
                                   uint32_t version)
{
   struct Display *d = data;

   if ( strcmp (interface, "wl_compositor") == 0 )
   {
      d->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 1);
   }
   if ( strcmp (interface, "wl_shell") == 0 )
   {
      d->shell = wl_registry_bind(registry, name, &wl_shell_interface, 1);
   }
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry,
                                          uint32_t name)
{
}

static const struct wl_registry_listener registry_listener = {
   registry_handle_global,
   registry_handle_global_remove
};

static void shell_ping(void *data, struct wl_shell_surface *shell_surface, uint32_t serial)
{
   wl_shell_surface_pong(shell_surface, serial);
}

static void shell_configure(void *data, struct wl_shell_surface *shell_surface,
                            uint32_t edges, int32_t width, int32_t height)
{
}

static void shell_popup_done(void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
   shell_ping,
   shell_configure,
   shell_popup_done
};


//////////////////////////////////////////////////////////////////
//
//  Public Functions
//
//

///
//  WinCreate()
//
//      This function initialized the native X11 display and window for EGL
//
EGLBoolean WinCreate(ESContext *esContext, const char *title)
{
    EGLConfig ecfg;
    EGLint num_config;

    /*
     * Wayland display initialization
     */

    wl_disp.display = wl_display_connect(NULL);
    if ( wl_disp.display == NULL )
    {
        printf("Failed to open display\n");
        return EGL_FALSE;
    }
    wl_disp.registry = wl_display_get_registry(wl_disp.display);

    if ( wl_disp.registry == NULL )
    {
        printf("Failed to get registry\n");
        return EGL_FALSE;
    }
    wl_registry_add_listener(wl_disp.registry, &registry_listener, &wl_disp);

    wl_display_roundtrip(wl_disp.display);

    wl_win.surface = wl_compositor_create_surface(wl_disp.compositor);
    if ( wl_win.surface == NULL )
    {
        printf("Failed to create surface\n");
        return EGL_FALSE;
    }

    wl_win.shell_surface = wl_shell_get_shell_surface(wl_disp.shell, wl_win.surface);

    if ( wl_win.shell_surface == NULL )
    {
        printf("Failed to create shell_surface\n");
        return EGL_FALSE;
       
    }

    wl_shell_surface_add_listener(wl_win.shell_surface, &shell_surface_listener, &wl_win);
    wl_shell_surface_set_title(wl_win.shell_surface, title);
    wl_shell_surface_set_toplevel(wl_win.shell_surface);

    wl_win.native = wl_egl_window_create(wl_win.surface, esContext->width, esContext->height);

    esContext->eglNativeWindow = (EGLNativeWindowType)wl_win.native;
    esContext->eglNativeDisplay = (EGLNativeDisplayType)wl_disp.display;
    return EGL_TRUE;
}

///
//  userInterrupt()
//
//      Reads from X11 event loop and interrupt program if there is a keypress, or
//      window close action.
//
GLboolean userInterrupt(ESContext *esContext)
{
    GLboolean userinterrupt = GL_FALSE;

    return userinterrupt;
}

///
//  WinLoop()
//
//      Start main windows loop
//
void WinLoop ( ESContext *esContext )
{
    struct timeval t1, t2;
    struct timezone tz;
    float deltatime;

    gettimeofday ( &t1 , &tz );

    while(userInterrupt(esContext) == GL_FALSE)
    {
        gettimeofday(&t2, &tz);
        deltatime = (float)(t2.tv_sec - t1.tv_sec + (t2.tv_usec - t1.tv_usec) * 1e-6);
        t1 = t2;

        if (esContext->updateFunc != NULL)
            esContext->updateFunc(esContext, deltatime);
        if (esContext->drawFunc != NULL)
            esContext->drawFunc(esContext);

        eglSwapBuffers(esContext->eglDisplay, esContext->eglSurface);        
    }
}

///
//  Global extern.  The application must declsare this function
//  that runs the application.
//
extern int esMain( ESContext *esContext );

///
//  main()
//
//      Main entrypoint for application
//
int main ( int argc, char *argv[] )
{
   ESContext esContext;
   
   memset ( &esContext, 0, sizeof( esContext ) );


   if ( esMain ( &esContext ) != GL_TRUE )
      return 1;   
 
   WinLoop ( &esContext );

   if ( esContext.shutdownFunc != NULL )
	   esContext.shutdownFunc ( &esContext );

   if ( esContext.userData != NULL )
	   free ( esContext.userData );

   return 0;
}
