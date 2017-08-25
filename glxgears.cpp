#define GST_USE_UNSTABLE_API

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/app/gstappsink.h>
#include <gst/app/gstappsrc.h>
#include <gst/gl/x11/gstgldisplay_x11.h>
#include <gst/gl/gstglmemory.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glxext.h>

#include <iostream>
#include <string>
#include "png_texture.h"

Display *dpy;
Window win;
GLXContext ctx;

GLuint normal_texture, gst_shared_texture;

#ifndef GLX_MESA_swap_control
#define GLX_MESA_swap_control 1
typedef int (*PFNGLXGETSWAPINTERVALMESAPROC)(void);
#endif

#include <sys/time.h>
#include <unistd.h>

/* return current time (in seconds) */
static double current_time(void) {
	struct timeval tv;
	struct timezone tz;
	(void) gettimeofday(&tv, &tz);
	return (double) tv.tv_sec + tv.tv_usec / 1000000.0;
}

#ifndef M_PI
#define M_PI 3.14159265
#endif

static GLfloat view_rotx = 20.0, view_roty = 30.0, view_rotz = 0.0;
static GLint gear1, gear2, gear3;
static GLfloat angle = 0.0;

static GLint samples = 0;           /* Choose visual with at least N samples. */
static GLboolean animate = GL_TRUE;	/* Animation */

void snapshot (guint input)  {

	GLint width,height,internalFormat;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, input);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_COMPONENTS, &internalFormat); // get internal format type of GL texture
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &width); // get width of GL texture
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &height); // get height of GL texture
	
	int numBytes = 0;
	
	if  (internalFormat == GL_RGBA8_EXT){
		printf("internal format is GL_RGBA8_EXT X: %d Y: %d \n",width,height);
		numBytes = width * height * 4;

		unsigned char *pixels = (unsigned char*)malloc(numBytes); 
		glGetTexImage(GL_TEXTURE_2D, 0, internalFormat, GL_UNSIGNED_BYTE, pixels);
		FILE *test = fopen("/home/pi/GL_RGBA8_EXT.raw", "wb");
		fwrite(pixels, 1, numBytes, test);
		fclose(test);
		free(pixels); 
	}
	glBindTexture(GL_TEXTURE_2D, 0);
	
}

/*
*
*  Draw a gear wheel.  You'll probably want to call this function when
*  building a display list since we do a lot of trig here.
* 
*  Input:  inner_radius - radius of hole at center
*          outer_radius - radius at center of teeth
*          width - width of gear
*          teeth - number of teeth
*          tooth_depth - depth of tooth
*/
static void gear(GLfloat inner_radius, GLfloat outer_radius, GLfloat width, GLint teeth, GLfloat tooth_depth)
{
	GLint i;
	GLfloat r0, r1, r2;
	GLfloat angle, da;
	GLfloat u, v, len;

	r0 = inner_radius;
	r1 = outer_radius - tooth_depth / 2.0;
	r2 = outer_radius + tooth_depth / 2.0;

	da = 2.0 * M_PI / teeth / 4.0;

	// glShadeModel(GL_FLAT);

	glNormal3f(0.0, 0.0, 1.0);

	/* draw front face */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		if (i < teeth) {
			glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
			glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
			width * 0.5);
		}
	}
	glEnd();

	/* draw front sides of teeth */
	glBegin(GL_QUADS);

	
	da = 2.0 * M_PI / teeth / 4.0;
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		width * 0.5);
	}
	glEnd();

	glNormal3f(0.0, 0.0, -1.0);


	/* draw back face */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		if (i < teeth) {
			glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
			-width * 0.5);
			glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		}
	}
	glEnd();

	/* draw back sides of teeth */
	glBegin(GL_QUADS);
	da = 2.0 * M_PI / teeth / 4.0;
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		-width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		-width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
	}
	glEnd();

	/* draw outward faces of teeth */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i < teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;

		glVertex3f(r1 * cos(angle), r1 * sin(angle), width * 0.5);
		glVertex3f(r1 * cos(angle), r1 * sin(angle), -width * 0.5);
		u = r2 * cos(angle + da) - r1 * cos(angle);
		v = r2 * sin(angle + da) - r1 * sin(angle);
		len = sqrt(u * u + v * v);
		u /= len;
		v /= len;
		glNormal3f(v, -u, 0.0);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), width * 0.5);
		glVertex3f(r2 * cos(angle + da), r2 * sin(angle + da), -width * 0.5);
		glNormal3f(cos(angle), sin(angle), 0.0);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		width * 0.5);
		glVertex3f(r2 * cos(angle + 2 * da), r2 * sin(angle + 2 * da),
		-width * 0.5);
		u = r1 * cos(angle + 3 * da) - r2 * cos(angle + 2 * da);
		v = r1 * sin(angle + 3 * da) - r2 * sin(angle + 2 * da);
		glNormal3f(v, -u, 0.0);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		width * 0.5);
		glVertex3f(r1 * cos(angle + 3 * da), r1 * sin(angle + 3 * da),
		-width * 0.5);
		glNormal3f(cos(angle), sin(angle), 0.0);
	}

	glVertex3f(r1 * cos(0), r1 * sin(0), width * 0.5);
	glVertex3f(r1 * cos(0), r1 * sin(0), -width * 0.5);

	glEnd();

	//  glShadeModel(GL_SMOOTH);

	/* draw inside radius cylinder */
	glBegin(GL_QUAD_STRIP);
	for (i = 0; i <= teeth; i++) {
		angle = i * 2.0 * M_PI / teeth;
		glNormal3f(-cos(angle), -sin(angle), 0.0);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), -width * 0.5);
		glVertex3f(r0 * cos(angle), r0 * sin(angle), width * 0.5);
	}
	glEnd();
}



static void draw_gears(void){
	
	// Move the camera back so we can see the board.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0, 0, -20);
	
	// Start with a clear screen
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_TEXTURE_2D);
	
	
	glPushMatrix();
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE,GL_REPLACE);
	glBindTexture (GL_TEXTURE_2D, gst_shared_texture); 
	
	int z_depth = -5;
	glBegin( GL_QUADS );
	glTexCoord2f( 0.0f, 0.0f );
	glVertex3f( -10.0f, 10.0f, z_depth );//top left
	
	glTexCoord2f( 0.0f, 1.0f ); 
	glVertex3f( -10.0f, -10.0f,z_depth );//bottom left
	
	glTexCoord2f( 1.0f, 1.0f );
	glVertex3f( 10.0f, -10.0f, z_depth );	//bottom right
	
	glTexCoord2f( 1.0f, 0.0f );
	glVertex3f( 10.0f, 10.0f, z_depth);	//top right
	glEnd( );

	glBindTexture (GL_TEXTURE_2D, 0); 
	
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPopMatrix();
	
	
	glRotatef(view_rotx, 1.0, 0.0, 0.0);
	glRotatef(view_roty, 0.0, 1.0, 0.0);
	glRotatef(view_rotz, 0.0, 0.0, 1.0);
	
	glPushMatrix();
	glTranslatef(-3.0, -2.0, 0.0);
	glRotatef(angle, 0.0, 0.0, 1.0);
	glCallList(gear1);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(3.1, -2.0, 0.0);
	glRotatef(-2.0 * angle - 9.0, 0.0, 0.0, 1.0);
	glCallList(gear2);
	glPopMatrix();

	glPushMatrix();
	glTranslatef(-3.1, 4.2, 0.0);
	glRotatef(-2.0 * angle - 25.0, 0.0, 0.0, 1.0);
	glCallList(gear3);
	glPopMatrix();
	
}


/** Draw single frame, do SwapBuffers, compute FPS */
static void draw_frame(Display *dpy, Window win)
{
	static int frames = 0;
	static double tRot0 = -1.0, tRate0 = -1.0;
	double dt, t = current_time();

	if (tRot0 < 0.0)
	tRot0 = t;
	dt = t - tRot0;
	tRot0 = t;

	if (animate) {
		/* advance rotation for next frame */
		angle += 70.0 * dt;  /* 70 degrees per second */
		if (angle > 3600.0)
		angle -= 3600.0;
	}

	draw_gears();
	glXSwapBuffers(dpy, win);
	
	frames++;

	if (tRate0 < 0.0)
	tRate0 = t;
	if (t - tRate0 >= 5.0) {
		GLfloat seconds = t - tRate0;
		GLfloat fps = frames / seconds;
		printf("Rendering %d frames in %3.1f seconds = %6.3f FPS\n", frames, seconds,fps);
		fflush(stdout);
		tRate0 = t;
		frames = 0;
	}
	
}

/* new window size or exposure */
static void reshape(int width, int height)
{
	glViewport(0, 0, (GLint) width, (GLint) height);

	//GLfloat h = (GLfloat) height / (GLfloat) width;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	//glFrustum(-1.0, 1.0, -h, h, 5.0, 60.0);
	
	float nearp = 1, farp = 500.0f, hht, hwd;
	hht = nearp * tan(45.0 / 2.0 / 180.0 * M_PI);
	hwd = hht * width / height;
	glFrustum(-hwd, hwd, -hht, hht, nearp, farp);
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.0, 0.0, -40.0);
}


static void init(void)
{
	
	//glEnable(GL_BLEND);
	//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	//glEnable(GL_TEXTURE_2D);
	
	glEnable(GL_DEPTH_TEST);


	static GLfloat pos[4] = { 5.0, 5.0, 10.0, 0.0 };
	static GLfloat red[4] = { 0.8, 0.1, 0.0, 1.0 };
	static GLfloat green[4] = { 0.0, 0.8, 0.2, 1.0 };
	static GLfloat blue[4] = { 0.2, 0.2, 1.0, 1.0 };
	glShadeModel(GL_FLAT);
	glLightfv(GL_LIGHT0, GL_POSITION, pos);
	glEnable(GL_CULL_FACE);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	
	/* make the gears */
	gear1 = glGenLists(1);
	glNewList(gear1, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, red);
	gear(1.0, 4.0, 1.0, 20, 0.7);
	glEndList();

	gear2 = glGenLists(1);
	glNewList(gear2, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green);
	gear(0.5, 2.0, 2.0, 10, 0.7);
	glEndList();

	gear3 = glGenLists(1);
	glNewList(gear3, GL_COMPILE);
	glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, blue);
	gear(1.3, 2.0, 0.5, 10, 0.7);
	glEndList();

	glEnable(GL_NORMALIZE);
}

static gboolean bus_call (GstBus *bus, GstMessage *msg, gpointer data)
{
	GMainLoop *loop = (GMainLoop*)data;

	switch (GST_MESSAGE_TYPE (msg))
	{
	case GST_MESSAGE_EOS:
		g_print ("End-of-stream\n");
		g_main_loop_quit (loop);
		break;
	case GST_MESSAGE_ERROR:
		{
			gchar *debug = NULL;
			GError *err = NULL;

			gst_message_parse_error (msg, &err, &debug);

			g_print ("Error: %s\n", err->message);
			g_error_free (err);

			if (debug)
			{
				g_print ("Debug deails: %s\n", debug);
				g_free (debug);
			}

			g_main_loop_quit (loop);
			break;
		}
	case GST_MESSAGE_NEED_CONTEXT:  //THIS IS THE IMPORTANT PART
		{			
			const gchar *context_type;
			GstContext *context = NULL;
			gst_message_parse_context_type (msg, &context_type);
			if (g_strcmp0 (context_type, "gst.gl.app_context") == 0) {
				g_print("OpenGL Context Request Intercepted! %s\n", context_type);	
				GstGLDisplay *sdl_gl_display = GST_GL_DISPLAY (gst_gl_display_x11_new_with_display (dpy));//dpy is the glx OpenGL display
				GstGLContext *gl_context = gst_gl_context_new_wrapped ( sdl_gl_display, (guintptr) ctx,GST_GL_PLATFORM_GLX,GST_GL_API_OPENGL); //ctx is the glx OpenGL context
				context = gst_context_new ("gst.gl.app_context", TRUE);
				GstStructure *s = gst_context_writable_structure (context);
				gst_structure_set (s, "context", GST_GL_TYPE_CONTEXT, gl_context, NULL);
				gst_element_set_context (GST_ELEMENT (msg->src), context);
			}
			if (g_strcmp0 (context_type, GST_GL_DISPLAY_CONTEXT_TYPE) == 0) {
				g_print("X11 Display Request Intercepted! %s\n", context_type);				
				GstGLDisplay *gl_display = GST_GL_DISPLAY (gst_gl_display_x11_new_with_display (dpy));//dpy is the glx OpenGL display			
				context = gst_context_new (GST_GL_DISPLAY_CONTEXT_TYPE, TRUE);
				gst_context_set_gl_display (context, gl_display);
				gst_element_set_context (GST_ELEMENT (msg->src), context);
			}
			if (context)
			gst_context_unref (context);
			break;
		}
	default:
		break;
	}

	return TRUE;
}


/*
* Create an RGB, double-buffered window.
* Return the window and context handles.
*/
static void make_window( Display *dpy, const char *name,int x, int y, int width, int height,Window *winRet, GLXContext *ctxRet, VisualID *visRet) {
	int attribs[64];
	int i = 0;

	int scrnum;
	XSetWindowAttributes attr;
	unsigned long mask;
	Window root;
	Window win;

	XVisualInfo *visinfo;

	/* Singleton attributes. */
	attribs[i++] = GLX_RGBA;
	attribs[i++] = GLX_DOUBLEBUFFER;

	/* Key/value attributes. */
	attribs[i++] = GLX_RED_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_GREEN_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_BLUE_SIZE;
	attribs[i++] = 1;
	attribs[i++] = GLX_DEPTH_SIZE;
	attribs[i++] = 1;
	if (samples > 0) {
		attribs[i++] = GLX_SAMPLE_BUFFERS;
		attribs[i++] = 1;
		attribs[i++] = GLX_SAMPLES;
		attribs[i++] = samples;
	}

	attribs[i++] = None;

	scrnum = DefaultScreen( dpy );
	root = RootWindow( dpy, scrnum );

	visinfo = glXChooseVisual(dpy, scrnum, attribs);
	if (!visinfo) {
		printf("Error: couldn't get an RGB, Double-buffered");
		if (samples > 0)
		printf(", Multisample");
		printf(" visual\n");
		exit(1);
	}

	/* window attributes */
	attr.background_pixel = 0;
	attr.border_pixel = 0;
	attr.colormap = XCreateColormap( dpy, root, visinfo->visual, AllocNone);
	attr.event_mask = StructureNotifyMask | ExposureMask | KeyPressMask;
	/* XXX this is a bad way to get a borderless window! */
	mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;

	win = XCreateWindow( dpy, root, x, y, width, height,
	0, visinfo->depth, InputOutput,
	visinfo->visual, mask, &attr );

	/* set hints and properties */
	{
		XSizeHints sizehints;
		sizehints.x = x;
		sizehints.y = y;
		sizehints.width  = width;
		sizehints.height = height;
		sizehints.flags = USSize | USPosition;
		XSetNormalHints(dpy, win, &sizehints);
		XSetStandardProperties(dpy, win, name, name,
		None, (char **)NULL, 0, &sizehints);
	}

	ctx = glXCreateContext( dpy, visinfo, NULL, True );
	if (!ctx) {
		printf("Error: glXCreateContext failed\n");
		exit(1);
	}

	*winRet = win;
	*ctxRet = ctx;
	*visRet = visinfo->visualid;

	XFree(visinfo);
}


/**
* Determine whether or not a GLX extension is supported.
*/
static int is_glx_extension_supported(Display *dpy, const char *query)
{
	const int scrnum = DefaultScreen(dpy);
	const char *glx_extensions = NULL;
	const size_t len = strlen(query);
	const char *ptr;

	if (glx_extensions == NULL) {
		glx_extensions = glXQueryExtensionsString(dpy, scrnum);
	}

	ptr = strstr(glx_extensions, query);
	return ((ptr != NULL) && ((ptr[len] == ' ') || (ptr[len] == '\0')));
}


/**
* Attempt to determine whether or not the display is synched to vblank.
*/
static void query_vsync(Display *dpy, GLXDrawable drawable)
{
	int interval = 0;

#if defined(GLX_EXT_swap_control)
	if (is_glx_extension_supported(dpy, "GLX_EXT_swap_control")) {
		unsigned int tmp = -1;
		glXQueryDrawable(dpy, drawable, GLX_SWAP_INTERVAL_EXT, &tmp);
		interval = tmp;
	} else
#endif
	if (is_glx_extension_supported(dpy, "GLX_MESA_swap_control")) {
		PFNGLXGETSWAPINTERVALMESAPROC pglXGetSwapIntervalMESA =
		(PFNGLXGETSWAPINTERVALMESAPROC)
		glXGetProcAddressARB((const GLubyte *) "glXGetSwapIntervalMESA");

		interval = (*pglXGetSwapIntervalMESA)();
	} else if (is_glx_extension_supported(dpy, "GLX_SGI_swap_control")) {
		/* The default swap interval with this extension is 1.  Assume that it
	* is set to the default.
	*
	* Many Mesa-based drivers default to 0, but all of these drivers also
	* export GLX_MESA_swap_control.  In that case, this branch will never
	* be taken, and the correct result should be reported.
	*/
		interval = 1;
	}


	if (interval > 0) {
		printf("Running synchronized to the vertical refresh.  The framerate should be\n");
		if (interval == 1) {
			printf("approximately the same as the monitor refresh rate.\n");
		} else if (interval > 1) {
			printf("approximately 1/%d the monitor refresh rate.\n",
			interval);
		}
	}
}



//client draw callback
gboolean drawCallback (GstElement* object, guint id , guint width ,guint height, gpointer user_data){

	static GTimeVal current_time;
	static glong last_sec = current_time.tv_sec;
	static gint nbFrames = 0;
	
	//printf("Texture #:%d  X:%d  Y:%d!\n",id,width,height);
	
	gst_shared_texture = id;
	g_get_current_time (&current_time);
	nbFrames++;

	if ((current_time.tv_sec - last_sec) >= 1)
	{
		std::cout << "GSTREAMER FPS = " << nbFrames << std::endl;
		nbFrames = 0;
		last_sec = current_time.tv_sec;
	}
	
	return true;
}


double last_update;


static gboolean worker_finish_in_idle (gpointer data) {
	draw_frame(dpy, win);
	return true; //automatically keep calling this function when idle
}

int main(int argc, char *argv[]){

	GstPipeline *pipeline;
	GstElement *testsink;

	gst_init (&argc, &argv);

	unsigned int winWidth = 300, winHeight = 300;
	int x = 0, y = 0;

	char *dpyName = NULL;
	GLboolean printInfo = GL_FALSE;
	VisualID visId;
	int max_size;
	
	dpy = XOpenDisplay(dpyName);
	if (!dpy) {
		printf("Error: couldn't open display %s\n",
		dpyName ? dpyName : getenv("DISPLAY"));
		return -1;
	}

	make_window(dpy, "glxgears", x, y, winWidth, winHeight, &win, &ctx, &visId);
	XMapWindow(dpy, win);
	glXMakeCurrent(dpy, win, ctx);
	query_vsync(dpy, win);

	//normal_texture = png_texture_load( "blue_0.png", NULL, NULL);
	
	//printf("Initial texture Number %d\n",normal_texture);
	
	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &max_size);
	
	if (printInfo) {
		printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
		printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
		printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
		printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
		printf("VisualID %d, 0x%x\n", (int) visId, (int) visId);
	}

	init();

	/* Set initial projection/viewing transformation.
	* We can't be sure we'll get a ConfigureNotify event when the window
	* first appears.
	*/
	reshape(winWidth, winHeight);

	pipeline = GST_PIPELINE (gst_parse_launch   ("videotestsrc ! video/x-raw,width=320,height=320,framerate=(fraction)20/1 ! glupload ! glfilterapp name=testsink ! fakesink sync=true", NULL));
	//pipeline = GST_PIPELINE (gst_parse_launch   ("udpsrc port=9000 caps=application/x-rtp ! rtpjpegdepay ! queue ! jpegdec ! queue ! videoconvert ! revtv ! videoconvert !  glupload ! glfilterapp name=testsink ! fakesink sync=false", NULL));
	//pipeline = GST_PIPELINE (gst_parse_launch   ("udpsrc port=9000 caps=application/x-rtp ! rtpjpegdepay ! queue ! jpegdec ! queue ! videoconvert ! glupload ! gleffects_heat ! glfilterapp name=testsink ! fakesink sync=false", NULL));
	//gst-launch-1.0 -vvv rpicamsrc ! video/x-raw,width=640,height=480,framerate=30/1 ! queue max-size-time=50000000 leaky=upstream ! jpegenc ! rtpjpegpay ! udpsink host=192.168.1.169 port=9000 sync=false
	
	//gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PAUSED);  //technically we need to pause before setting callback?
	
	testsink = gst_bin_get_by_name (GST_BIN (pipeline), "testsink");
	g_signal_connect (testsink, "client-draw",  G_CALLBACK (drawCallback), NULL);  //IMPORTANT!
	gst_object_unref (testsink);
	
	gst_element_set_state (GST_ELEMENT (pipeline), GST_STATE_PLAYING);
	
	GMainLoop *loop;
	GstBus *bus;

	loop = g_main_loop_new (NULL, FALSE);
	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus, bus_call, loop);
	gst_object_unref (bus);
	
	gpointer data = NULL;
	g_idle_add (worker_finish_in_idle, data);
	g_main_loop_run (loop);
	
	glDeleteLists(gear1, 1);
	glDeleteLists(gear2, 1);
	glDeleteLists(gear3, 1);
	glXMakeCurrent(dpy, None, NULL);
	glXDestroyContext(dpy, ctx);
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);

	return 0;
}