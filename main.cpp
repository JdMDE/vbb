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

// config.h does not need to be explicitly included, since canvas and pdfslides include it.
#include "canvas.h"
#include "pdfslides.h"

/**
 * The entry point of the program
 * \param argc The number of arguments. It will be checked that it is 1 (the program name) or 2 (the program name and the PDF file to load, if any)
 * \param argv The argument. argv[0] will be the name of the program. argv[1] the name of the .pdf file with the slides (if present).
 * \return 0 on success, 1 on abormal or premature exit by error
 */
int main(int argc,char *argv[])
{
 if (argc>2)
 {
  std::cerr << "Usage: " << argv[0] << " [pdf_file]\n";
  std::cerr << "       If no pdf file is given, an empty blackboard is opened.\n";
  std::cerr << "       This program has no other command-line parameters.\n";
  std::cerr << "       Configuration is done via config files, either \n";
  std::cerr << "          '$HOME/" << Config::ConfigFileNameLocal << "' or\n";
  std::cerr << "          '" << Config::DefaultGlobalConfigDir << Config::ConfigFileNameGlobal << "'\n";
  std::cerr << "       in that order of preference.\n";
  exit(1);
 }
 
 // The name of the pdf file to be loaded, or the empty string if no one is passed (empty blackboard)
 std::string fname=(argc==2) ? std::string(argv[1]) : "";
 
 // The configuration object is populated with the values from the configuration files 
 Config cfg;

 // A canvas is created, according to the values stored in config
 Canvas cnv(cfg);
 
 // A PSDSlides structure if filled with the characteristics of the splash file (if needed) and PDF file (if read)
 PDFSlides sld(cfg,fname);
 
 // This loads the splash screen (if requested), draws the upper menu (always) and the first slide (it there are slides). Then, it redraws the canvas.
 cnv.Prepare(cfg,sld.GetSplashSurface(),sld.GetCurrentPageSurface());
 
 // These are the variable for the main loop whose values will change at any turn according to the user's mouse clicks or key presses.
 SDL_Event ev;
 Config::Commands command=Config::NoCommand;
 bool sent_to_canvas=true;
 
 while (command!=Config::Quit)
 {
  // All keyboard or mouse events are read, but only those relevant will be processed
  while (SDL_WaitEvent(&ev) && (command!=Config::Quit))
  {
   // In principle, the event does not call for any command...   
   command=Config::NoCommand;
   switch (ev.type)
   {
    // A mouse or pen button (no matters which one) has been pressed.
    case SDL_MOUSEBUTTONDOWN:
                // If the click is on the drawing area...
                if (cnv.InsideCanvas(ev.button.y))
                {
                 // ... we will start tracing a line which will follow the mouse/pen, First, internal state becomes 'Tracing'...
                 cnv.SetTracing(true);
                 // ... and then the internal coordinates, which will be the initial point of the line, are stored.
                 cnv.SetCoords(ev.button.x,ev.button.y);
                }
                else
                 // If click is on menu, this function returns the command associated, according to the location.
                 // This command is to be sent to the Canvas object or to the PDFSlides object. The boolean variable takes care of that.
                 // All commands of the menu are for Canvas, except Next and Previous
                 command=cnv.GetPosCode(ev.button.x,ev.button.y,sent_to_canvas);
                break;
    // A mouse button is released. This mean the pen/mouse must stop tracing (drawing line when it is moved)
    case SDL_MOUSEBUTTONUP:
               if (cnv.InsideCanvas(ev.button.y))
                cnv.SetTracing(false);
               break;  
    // The mouse/pen is being moved
    case SDL_MOUSEMOTION:
               // If we are moving while the mouse/pen button is pressed (Tracing mode), a line is drawn from the internal coordinates of start to the current point.
               // After tracing, drawline updates the internal current coordinates, instead of calling here SetCoords. This is just for efficiency.
                if (cnv.GetTracing()==true)
                 cnv.Drawline(ev.button.x,ev.button.y);
                break;
    // A key has been pressed
    case SDL_KEYDOWN:
               // If a key is pressed, the table of correspondances key <--> command internally stored in the configuration object (accelerator keys) is consulted
               // Any key not in the table will return NoCommand.
                command=cfg.InterpretKey(ev.key.keysym.sym,sent_to_canvas);
                break;
    // All other events (key releases, for example) are ignored.
    default: break;
   }
   
   // The follwoing lines will efectively execute the received commands, it there is something to execute.
   if ((command!=Config::NoCommand) && (command!=Config::Quit))
   {
    // In the case of commands for the Canvas, it is the Canvas object itself which does the redraw, as needed (only of the slides, the traces, or both things).
    // This is tricky so it is better to do it inside the canvas, where all these things are accessible.
    if ( sent_to_canvas )
     cnv.ExecuteCommand(command,sld.GetCurrentPageSurface());
    else
     // In the case of commans for the PDFSLides, in general, it will always need redraw, unless the command has not been executed
     // (for example: trying to advance after the last slide, or before the first). ExecuteCommand returns a boolean to know that.
     // This is why ExecuteCommand is a function and not a void, as in the Canvas object.
     if (sld.ExecuteCommand(command))
     {
      cnv.Erase(Canvas::Slide);
      cnv.Show(sld.GetCurrentPageSurface());
      cnv.Merge();
      cnv.Update(Canvas::Both);
     }
   }
  }
 }
 // We have left the loop by generating the Quit command. 
 cnv.EndSDL();

 // Return success (this is the intended way to leave the program).
 return 0;
}

