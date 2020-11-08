/***************************************************************************
 *   Copyright (C) 2020 by Juan Domingo Esteve                             *
 *   Juan.Domingo@uv.es                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <https://www.gnu.org/licenses/>. *
 ***************************************************************************/
#ifndef CANVAS_H
#define CANVAS_H

#include "config.h"

// All include needed hare are already included by config.h, except SDL.h, SDL_image.h and SDL_ttf.h
// but SDL.h and SDL_image.h are already included by SDL_ttf.h
#include <SDL/SDL_ttf.h>

#include <SDL/SDL_syswm.h>
/*! \brief Class to manage the graphical SDL surface(s) that are being displayed and their overlays.
 *
 * This class is constructed with the configuration file name as argument, since it needs several values
 * that were obtained by the config object from the configuration file.
 *         
 * The Canvas object (just one per program) takes care of the graphical stuff. Internally, it maintains two
 * different drawing surfaces, one to contain the slides and another to contain the user's traces done with
 * the pen. The inconvenient is that they have to be appropriately merged before being shown; the advantages
 * are that they can be shown or erased sepparately, which is a good feature for presentations.
 *
*/
class Canvas
{
 public:
    /** 
     * Possible colors to be used. Each is defined inside by its RGB values.
     *
     *  WARNING: if you want to add more colors, keep always NumCols as last element, and DON'T change default values (0,1,...)
     */
    enum ColDefs { Red, Green, Blue, Black, White, PaleBlue, Yellow, Orange, NumCols };

    /**
     * The color of the eraser
     */
    const int Ecol = White;

    /** 
     * Marks for the things that can need updating at some time: the surface where the pdf slides are drawn, the buffer where the user draws or both things.
     */
    enum UpdatableObjects { Slide, Buffer, Both };

    /**
     * Possible modes of working at any moment (only two: we can be drawing, or erasing).
     */
    enum Modes { Drawing, Erasing };

    /**
     * Constructor
     * \param cfg A reference to the config object that contains the values got from the configuration file
     *
     * NOTE: No destructor is needed. Free surface of internal SDL surfaces *c and *buf is done by SDL_Quit.
     */
    Canvas(Config &cfg);
    
    /**
     * Destructor. Nothing to do in it, since free surface of internal SDL surfaces *c and *buf is done by SDL_Quit.
     * It is nevertheless necessary to avoid warnings because the -Winline flag.
     */
    ~Canvas() {};
    
    /**
     * It shows in the window or screen the surface that is passed
     * \param s The surface to be drawn
     *
     * Side effects: once shown, a call to SDL_FreeSurface is done on the received variable, so that memory is kept at any moment low.
     */
    void Show(SDL_Surface *s);

    /**
     * It updates the thing or things that needs to be updated/redrawn. 
     * \param what One value of Slide, Buffer or Both
     */
    void Update(UpdatableObjects what);

    /**
     * It erases the thing or things that needs to be erased.
     * \param what One value of Slide, Buffer or Both
     */
    void Erase(UpdatableObjects what);

    /**
     * Function to know if a point (mouse click) is inside canvas or over any menu item.
     * \param y Y coordinate of click
     * \return true is click is inside canvas, false if it is inside menu
     */
    bool InsideCanvas(int y) { return (y>menu_height); };
    
    /**
     * This function returns a command to be executed, according to the coordinates in which user has clicked. If the click is on one element of the menu, it decides which one. If it is on the drawinf area, it is used to trace lines or erase pixels.
     * \param x The x coordinate of the mouse click
     * \param y The y coordinate of the mouse click
     * \param to_canvas A reference to be returned as true if the command is for a Canvas object and false if it is for a PDFSlide object
     * \return A command, as given by the enumeration Commands in this class.
     */
    Config::Commands GetPosCode(int x,int y,bool &to_canvas);
    
    /**
     * Procedure to set the tracing mode.
     * \param b true to start tracing (i.e.: a line is drawn following the movement of the pen), false to stop tracing mode.
     */
    void SetTracing(bool b)   { tracing=b; TracingSetcolor(); return; };
    
    /**
     * Get the current state of the tracing mode
     * \return true if tracing mode is activated, false if not
     */
    bool GetTracing(void)     { return tracing; };
    
    /** 
     * Procedure to set the current coordinates of the pen. Used to start a line when entering the tracing mode.
     * \param x Value of coordinate x to be set
     * \param y Value of coordinate y to be set
     * 
     * Side effect: the internal state is updated with the new coordinates.
     */
    void SetCoords(int x,int y) { x0=x; y0=y; return; };
    
    /**
     * Procedure to draw a straight line from the current pen position to the requested point
     * \param x1 Value of coordinate x of the end of the line
     * \param y1 Value of coordinate y of the end of the line
     */
    void Drawline(int x1,int y1);
    
    /**
     * Procedure to merge the two surfaces (slides and pen traces) and show them in the window or screen. The traces are set after the slides, so they are always seen
     */
    void Merge(void);
    
    /**
     * Procedure to show the splash screen
     */
    void ShowSplash(SDL_Surface *spls);

    /** 
     * Procedure to draw the upper menu
     * \param mitems A vector of strings with the texts to be written in each menu entry, ordered as they must be shown.
     */
    void SetMenu(const std::vector<std::string> &mitems);
    
    /**
     * Procedure to be called at the end of the program to close gracefully the SDL library and free the used surfaces.
     */
    void EndSDL(void) { SDL_Quit(); }
  
    /** 
     * Procedure to execute a command requested by main
     * \param c Command to be executed
     * \param s SDL_Surface to be redrawn, if needed
     */
    void ExecuteCommand(Config::Commands com,SDL_Surface *s);
    
    /**
     * Procedure to prepare the canvas at the initial state. 
     * 
     * It shows the splash screen if the configuration requests it, sets up the menu in the upper part, shows the firs slide (if there are slides) and updates the canvas.
     * 
     * \param cfg The configuration object with relevant data (resolution, and others)
     * \param sp  Pointer to the surface of the splash screen, or null if no splash screen is to be shown
     * \param sl  Pointer to the surface of the first slide, or null if no slides have been loaded
     */
     void Prepare(Config &cfg,SDL_Surface *sp,SDL_Surface *sl);
    
 private:
    static const int MinLDis = 4;
    static const int MaxLWidth = 8;
    
    inline bool Inside(int x,int y,SDL_Rect &r) { return ((x>=r.x) && (x<=r.x+r.w) && (y>=r.y) && (y<=r.y+r.h)); };

    void DrawConfirmBoxAndWait(const std::string &fn,const std::string &message);
    
    void TextWithHighlight(const std::string &s,int r,int c,int d,int h);
    
    void ToggleDrawmode(void) { if (drawstate==Drawing) drawstate=Erasing; else drawstate=Drawing; DrawmodeSetcolor(); };
    
    void ChangeLineCharac(void);
    
    /*
     * Procedure to show the box that asks for user confirmation to save a blackboard as a graphical file and, if confirmed, does it
     * This procedure shows a message with the confirmation of saving or an error if the file has not been saved. Both messages are got
     * from the configuration file and atored inside canvas in the constructor.
     */
    void SaveBlackboard(void);
    
    // Changes the color of the small square in the upper-left corner form red to green when erasing or drawing
    void DrawmodeSetcolor(void);
    
    // Changes tho color of the even smaller squere inside the former one from black to red/green when tracing or not
    void TracingSetcolor(void);
    
    // Auxiliary function to draw a rectangle filled with the erase color
    void Drawrect(SDL_Rect r);
        
    // Auxiliary function to initialize som variables used to draw the line characteristics choice box
    void InitLC(void);
    
    // Procedure to write the current canvas (slides plus traces) to a pnm file 
    void WritePnm(std::ofstream &f);

    // The following internal variables are initialized in the constructor
    int last_saved;
    int scw,sch;
    int menu_height;
    SDL_Surface *c;
    SDL_Surface *buf;
    SDL_Color lc[NumCols];
    Uint32 line_draw_index,line_draw_col,line_erase_col;
    
    TTF_Font *tf;
    
    Modes drawstate;
    bool tracing;
    int line_width;
    int er_size;
    
    std::string save_message;
    std::string errorsave_message;
    
    unsigned char vback[8];
    
    // Initialized in constructor, but changed when menu is set 
    int num_menuitems;
    
    // Initialized in constructor, but changed the first time pen starts a trace.
    int x0,y0;
    
    // Variables used to draw the box of line characteristics selection. Initialized by function InitLC, called by constructor.
    int sqside;
    int xoff,yoff;
    int rw,rh;
 
    SDL_Rect choicerect,colrect[NumCols],linerect[MaxLWidth],lines[MaxLWidth],ok,defcol,defline;
 
    SDL_Surface *text;
 
    SDL_Rect oktext;
};

#endif
