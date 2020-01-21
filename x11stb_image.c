


//portions from 
//http://www.xmission.com/~georgeps/documentation/tutorials/Xlib_Beginner.html

//#define HAS_XINERAMA
//#define HAS_XSHAPE
//#define FULL_SCREEN_STEAL_FOCUS


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

#include "CNFGFunctions.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <X11/keysym.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

int setx = -1;
int sety = -1;
int show_decoration = 1;
int close_disable = 0;


uint32_t CNFGLastColor;
uint32_t CNFGBGColor;
int quit;
XWindowAttributes CNFGWinAtt;
XClassHint *CNFGClassHint;
Display *CNFGDisplay;
Window CNFGWindow;
int CNFGWindowInvisible;
Pixmap CNFGPixmap;
GC     CNFGGC;
GC     CNFGWindowGC;
Visual * CNFGVisual;

void CNFGGetDimensions( short * x, short * y )
{
	*x = CNFGWinAtt.width;
	*y = CNFGWinAtt.height;
}

void	CNFGChangeWindowTitle( const char * WindowName )
{
	XSetStandardProperties( CNFGDisplay, CNFGWindow, WindowName, 0, 0, 0, 0, 0 );
}


static void InternalLinkScreenAndGo( const char * WindowName )
{
	XFlush(CNFGDisplay);
	XGetWindowAttributes( CNFGDisplay, CNFGWindow, &CNFGWinAtt );

	XSelectInput (CNFGDisplay, CNFGWindow, KeyPressMask | KeyReleaseMask  /* | ButtonPressMask | ButtonReleaseMask | */ | ExposureMask /* | PointerMotionMask */ );

	CNFGWindowGC = XCreateGC(CNFGDisplay, CNFGWindow, 0, 0);

	CNFGPixmap = XCreatePixmap( CNFGDisplay, CNFGWindow, CNFGWinAtt.width, CNFGWinAtt.height, CNFGWinAtt.depth );
	CNFGGC = XCreateGC(CNFGDisplay, CNFGPixmap, 0, 0);
	XSetLineAttributes(CNFGDisplay, CNFGGC, 1, LineSolid, CapRound, JoinRound);
	CNFGChangeWindowTitle( WindowName );

	if( !show_decoration )
	{
		Atom window_type = XInternAtom( CNFGDisplay, "_NET_WM_WINDOW_TYPE", False);
		long value = XInternAtom( CNFGDisplay, "_NET_WM_WINDOW_TYPE_DOCK", False);
		XEvent e;
		XChangeProperty(CNFGDisplay, CNFGWindow, window_type, XA_ATOM, 32, PropModeReplace, (unsigned char *) &value, 1);
	}

	if( !CNFGWindowInvisible )
		XMapWindow(CNFGDisplay, CNFGWindow);

	if( setx > 0 || sety > 0 )
	{
		if( setx < 0 ) setx = 0;
		if( sety < 0 ) sety = 0;
		XMoveWindow( CNFGDisplay, CNFGWindow, setx, sety );
	}
}



void CNFGTearDown()
{
	if ( CNFGClassHint ) XFree( CNFGClassHint );
	if ( CNFGGC ) XFreeGC( CNFGDisplay, CNFGGC );
	if ( CNFGWindowGC ) XFreeGC( CNFGDisplay, CNFGWindowGC );
	if ( CNFGDisplay ) XCloseDisplay( CNFGDisplay );
	CNFGDisplay = NULL;
	CNFGWindowGC = CNFGGC = NULL;
	CNFGClassHint = NULL;
}

int CNFGSetup( const char * WindowName, int w, int h )
{
	CNFGDisplay = XOpenDisplay(NULL);
	if ( !CNFGDisplay ) {
		fprintf( stderr, "Could not get an X Display.\n%s", 
				 "Are you in text mode or using SSH without X11-Forwarding?\n" );
		exit( 1 );
	}
	atexit( CNFGTearDown );

	int screen = DefaultScreen(CNFGDisplay);
	int depth = DefaultDepth(CNFGDisplay, screen);
 	CNFGVisual = DefaultVisual(CNFGDisplay, screen);
	Window wnd = DefaultRootWindow( CNFGDisplay );

	XSetWindowAttributes attr;
	attr.background_pixel = 0;
	attr.colormap = XCreateColormap( CNFGDisplay, wnd, CNFGVisual, AllocNone);
	if( w && h )
	{
		CNFGWindow = XCreateWindow(CNFGDisplay, wnd, 0, 0, w, h, 0, depth, InputOutput, CNFGVisual, CWBackPixel | CWColormap, &attr );
	}
	else
	{
		CNFGWindow = XCreateWindow(CNFGDisplay, wnd, 0, 0, 1, 1, 0, depth, InputOutput, CNFGVisual, CWBackPixel | CWColormap, &attr );
		CNFGWindowInvisible = 1;
	}

	InternalLinkScreenAndGo( WindowName );

	return 0;
}

void X11STBImageHandleInput()
{
	if( !CNFGWindow ) return;
	static int ButtonsDown;
	XEvent report;

	int bKeyDirection = 1;
	//while( XPending( CNFGDisplay ) )
	{
		XNextEvent( CNFGDisplay, &report );

		bKeyDirection = 1;
		switch  (report.type)
		{
		case NoExpose:
			break;
		case Expose:
			XGetWindowAttributes( CNFGDisplay, CNFGWindow, &CNFGWinAtt );
			if( CNFGPixmap ) XFreePixmap( CNFGDisplay, CNFGPixmap );
			CNFGPixmap = XCreatePixmap( CNFGDisplay, CNFGWindow, CNFGWinAtt.width, CNFGWinAtt.height, CNFGWinAtt.depth );
			if( CNFGGC ) XFreeGC( CNFGDisplay, CNFGGC );
			CNFGGC = XCreateGC(CNFGDisplay, CNFGPixmap, 0, 0);
			break;
		case KeyRelease:
			bKeyDirection = 0;
		case KeyPress:
			//g_x_global_key_state = report.xkey.state;
			//g_x_global_shift_key = XLookupKeysym(&report.xkey, 1);
			//HandleKey( XLookupKeysym(&report.xkey, 0), bKeyDirection );
			//exit( 0 );
			if( bKeyDirection && !close_disable ) quit = 1;
			break;
		case ButtonRelease:
			bKeyDirection = 0;
		case ClientMessage:
			// Only subscribed to WM_DELETE_WINDOW, so just exit
			exit( 0 );
			break;
		default:
			break;
			//printf( "Event: %d\n", report.type );
		}
	}
}


void CNFGUpdateScreenWithBitmap( unsigned long * data, int w, int h )
{
	static XImage *xi;
	static int depth;
	static int lw, lh;

	if( !xi )
	{
		int screen = DefaultScreen(CNFGDisplay);
		depth = DefaultDepth(CNFGDisplay, screen)/8;
//		xi = XCreateImage(CNFGDisplay, DefaultVisual( CNFGDisplay, DefaultScreen(CNFGDisplay) ), depth*8, ZPixmap, 0, (char*)data, w, h, 32, w*4 );
//		lw = w;
//		lh = h;
	}

	if( lw != w || lh != h )
	{
		if( xi ) free( xi );
		xi = XCreateImage(CNFGDisplay, CNFGVisual, depth*8, ZPixmap, 0, (char*)data, w, h, 32, w*4 );
		lw = w;
		lh = h;
	}

	XPutImage(CNFGDisplay, CNFGWindow, CNFGWindowGC, xi, 0, 0, 0, 0, w, h );
}


uint32_t CNFGColor( uint32_t RGB )
{
	unsigned char red = RGB & 0xFF;
	unsigned char grn = ( RGB >> 8 ) & 0xFF;
	unsigned char blu = ( RGB >> 16 ) & 0xFF;
	CNFGLastColor = RGB;
	unsigned long color = (red<<16)|(grn<<8)|(blu);
	XSetForeground(CNFGDisplay, CNFGGC, color);
	return color;
}

void CNFGClearFrame()
{
	XGetWindowAttributes( CNFGDisplay, CNFGWindow, &CNFGWinAtt );
	XSetForeground(CNFGDisplay, CNFGGC, CNFGColor(CNFGBGColor) );	
	XFillRectangle(CNFGDisplay, CNFGPixmap, CNFGGC, 0, 0, CNFGWinAtt.width, CNFGWinAtt.height );
}

void CNFGSwapBuffers()
{
#ifdef HAS_XSHAPE
	if( taint_shape )
	{
		XShapeCombineMask(CNFGDisplay, CNFGWindow, ShapeBounding, 0, 0, xspixmap, ShapeSet);
		taint_shape = 0;
	}
#endif
	XCopyArea(CNFGDisplay, CNFGPixmap, CNFGWindow, CNFGWindowGC, 0,0,CNFGWinAtt.width,CNFGWinAtt.height,0,0);
	XFlush(CNFGDisplay);
#ifdef FULL_SCREEN_STEAL_FOCUS
	if( FullScreen )
		XSetInputFocus( CNFGDisplay, CNFGWindow, RevertToParent, CurrentTime );
#endif
}

void CNFGTackSegment( short x1, short y1, short x2, short y2 )
{
	XDrawLine( CNFGDisplay, CNFGPixmap, CNFGGC, x1, y1, x2, y2 );
	XDrawPoint( CNFGDisplay, CNFGPixmap, CNFGGC, x2, y2 );
}

void CNFGTackPixel( short x1, short y1 )
{
	XDrawPoint( CNFGDisplay, CNFGPixmap, CNFGGC, x1, y1 );
}

void CNFGTackRectangle( short x1, short y1, short x2, short y2 )
{
	XFillRectangle(CNFGDisplay, CNFGPixmap, CNFGGC, x1, y1, x2-x1, y2-y1 );
}

void CNFGTackPoly( RDPoint * points, int verts )
{
	XFillPolygon(CNFGDisplay, CNFGPixmap, CNFGGC, (XPoint *)points, 3, Convex, CoordModeOrigin );
}

void CNFGInternalResize( short x, short y ) { }

void CNFGSetLineWidth( short width )
{
	XSetLineAttributes(CNFGDisplay, CNFGGC, width, LineSolid, CapRound, JoinRound);
}



int main( int argc, char ** argv )
{
	int c;
	const char * title = 0;
	const char * fname = 0;
	int setw = -1;
	int seth = -1;
	CNFGBGColor = 0x800000;
	int showhelp = 0;
	int index;
	while( (c = getopt( argc, argv, "?t:w:h:hx:y:dc:" )) != -1 )
	{
		switch( c )
		{
			case 'b':
				CNFGBGColor = atoi( optarg ); 
				break;
			case 't': 
				title = optarg;
				break;
			case 'h':
				seth = atoi( optarg );
				break;
			case 'w':
				setw = atoi( optarg );
				break;
			case 'd':
				show_decoration = 0;
				break;
			case 'x':
				setx = atoi( optarg );
				break;
			case 'y':
				sety = atoi( optarg );
				break;
			case 'c':
				close_disable = 1;
				break;
			case '?':
				showhelp = 1;
				break;
		}
	}

	for (index = optind; index < argc; index++)
		fname = argv[index];

	if( !fname || showhelp )
	{
		fprintf( stderr, "x11stb_image: Usage: x11stb_image [-t title] [-w width] [-h height] [-d] [-x x] [-y y] [-c] filename\n\t-d is decorations (0/1), file loaded is the last parameter.\n\tPress any key to close, -c disables this behavior.\n\tSupported types (From STB): JPEG, PNG, TGA, BMP, PSD, GIF (no animation), HDR, PIC, PNM\n" );
		exit( -1 );
	}
	if( title == 0 ) title = fname;
	int imgx, imgy, imgn = 0;
	unsigned char * data = stbi_load( fname, &imgx, &imgy, &imgn, 4 );
	
//    int x,y,n;
//    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
//    // ... process data if not NULL ...
//    // ... x = width, y = height, n = # 8-bit components per pixel ...
//    // ... replace '0' with '1'..'4' to force that many components per pixel
//    // ... but 'n' will always be the number that it would have been if you said 0


	if( data )
	{
		if( setw < 0 ) setw = imgx;
		if( seth < 0 ) seth = imgy;
	}
	else
	{
		if( setw < 0 || seth < 0 ) setw = seth = 500;
	}

	CNFGSetup( title, setw, seth );
	
	CNFGClearFrame();
	CNFGSwapBuffers();
	CNFGClearFrame();
	CNFGSwapBuffers();
	while( !quit )
	{
		if( !data )
		{
			CNFGClearFrame();
			CNFGColor( 0xffffff );
			CNFGPenX = 10;
			CNFGPenY = 10;
			CNFGDrawText( "Could not load file.\n", 10 );
			CNFGPenY = 200;
			CNFGDrawText( fname	, 10 );
			CNFGSwapBuffers();
		}
		else
		{
			CNFGUpdateScreenWithBitmap( (unsigned long*)data, imgx, imgy );
		}

		X11STBImageHandleInput();
	}
	//Code never called?
	stbi_image_free(data);
	return 0;
}

