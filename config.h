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
#ifndef STDCONFIG_H
#define STDCONFIG_H

#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <unistd.h>
#include <SDL/SDL_keysym.h>

/*! \brief Class to read and parse the configuration files
 *         This class is constructed with the configuration file name as argument
 *         and also reads and parses the menu language configuration file indicated in it.
 * 
 * The Config object (just one per program) will read the global or local configuration
 * file and, if it is correct, will store its values that can later be retieved by other classes
 * with appropriate Get-methods. There are no setters for this class, since internal values are
 * read from files and not directly chosen, except for the screen resolution, that will be
 * obtained from the system at runtime if the program starts in full-creen mode.
 *
*/
class Config
{
 public:
    /**
     * Default value for the directory where read-only global configuration files can be found
     */
    static constexpr const char* DefaultGlobalConfigDir= "/etc/vbb/";
    
    /**
     * Default value for the name of the local configuration file (has preference)
     */
    static constexpr const char* ConfigFileNameLocal = ".vbb.cfg";

    /**
     * Default value for the name of the global configuration file (consulted if local file not present or readable)
     */
    static constexpr const char* ConfigFileNameGlobal = "vbb.cfg";

    /**
     * Default value for the window horizontal resolution in pixels
     */
    static const unsigned DefaultXRes = 1024;

    /**
     * Default value for the window vertical resolution in pixels
     */
    static const unsigned DefaultYRes = 576;

    /**
     * Default value for the name of the file with the splash screen (that shoud live in the DefaultGlobalConfigDir)
     */
    static constexpr const char* DefaultSplashFile = "vbb_splash.pdf";

    /**
     * Default value for teh directory containing the TTF fonts (at least one)
     */
    static constexpr const char* DefaultFontDir = "/usr/share/fonts/liberation/mono";

    /**
     * Default value for the TTF font used to write menu items and messages
     */
    static constexpr const char* DefaultFont = "LiberationMono-Bold.ttf";

    /**
     * Default value for the size of the TTF font used to write menu items and messages
     */
    static const unsigned DefaultFontSize = 12;

    /**
     * Default value for the size (either radius or square side) of the eraser around the cursor in Erase mode
     */
    static const unsigned DefaultEraserSize = 3;
    
    /**
     * Possible shapes of the eraser, either circular (EraserShapeCircle) or square (EraserShapeSquare)
     */
    enum EraserShapes { EraserShapeSquare, EraserShapeCircle };

    /**
     * Default value for the shape or the eraser, which must be one of those in the EraserShapes enumeration
     */
    static const EraserShapes DefaultEraserShape = EraserShapeCircle;
    
    /**
     * Default value for the name of the local language configuration file (has preference)
     */
    static constexpr const char* LangFileNameLocal = ".vbb_menu";
    
    /**
     * Default value for the name of the global language configuration file (that shoud live in the DefaultGlobalConfigDir)
     */
    static constexpr const char* LangFileNameGlobal = "vbb_menu";   
    
     /** 
      * Possible values to be returned when an option is parsed.
      * 
      * An option in the form Parameter: value can be:
      * 
      * valid (ValidPair)
      * 
      * invalid because the parameter is not in the list of expected parameters, (InvalidParam)
      * 
      * invalid the value is not a valid one for that parameter (InvalidValue), or both simultaneously.
      */
    enum ValidityValues { InvalidParam, InvalidValue, ValidPair };
    
    /**
     * List of keys to represent the accepted configuration parameters
     * 
     * UnknownParam: the user has written a parameter not accepted
     * 
     * OpenInWindow: should the program be opened in full-screen mode or in a window?
     *
     * XRes: resolution in pixels along X-axis, if not in full screen
     *
     * YRes: Resolution in pixels along Y-axis, if not in full screen
     *
     * EraserSize: size in pixels of the spot which will erase anything under it when the cursor moves it
     *
     * EraserShape: shape of teh eraser stop, either square or circular
     * 
     * FontDir: directory of the TTF fonts to be used to write the menu and messages
     *
     * FontName: name of the TTF font file to be used to write the menu and messages
     * 
     * FontSize: size in points of the TTF font file to be used to write the menu and messages
     *
     * LangFile: file with the text of the menu and accelerator keys in the user's language
     * 
     * SplashFile: file with the initial banner to be shown at program start, or None for not showing any banner at all
     */
    enum ConfigParams { UnknownParam, OpenInWindow, XRes, YRes, EraserSize, EraserShape, FontDir, FontName, FontSize, LangFile, SplashFile };
    
    /** 
     * The strings thet will have to be found as parameters in the configuration file and its association with constant enumerated values.
     */
    const std::map<std::string, Config::ConfigParams> ParamStrings {
        { "a",			    UnknownParam },
        { "OpenInWindow",   OpenInWindow },
        { "XRes",		    XRes },
        { "YRes",		    YRes },
        { "EraserSize",		EraserSize },
        { "EraserShape",	EraserShape },
        { "FontDir",		FontDir },
        { "FontName",		FontName },
        { "FontSize",		FontSize },
        { "LangFile",		LangFile },
        { "SplashFile",		SplashFile }
    };

    /**
     * Possible commands that either a Canvas object or a PDFSlide object can receive and shoudle execute.
     *
     * The commands which appear in the menu (those which are visible) must be mentioned in the same order as they are shown.
     * The slide-change commands (FF,FB,First,Last) will be associated by default to PageUp,PageDown,UpArrow and DownArrow respectively.
     * 
     * The intended recipient (Canvas or PDFSlide) is shown into ()
     * 
     * DrawErase: Toggle between draw and erase modes (Canvas)
     * 
     * LineCharac: Changes the characteristics of the drawing line (Canvas)
     * 
     * Next: Advances to the next slide (PDFSlide)
     * 
     * Previous: Goes back to the former slide (PDFSlide)
     * 
     * EraseAll: Erases the slide and the user traces (Canvas)
     * 
     * EraseSlide: Erases the slide but leave the user traces (Canvas)
     * 
     * EraseBlackb: Erases the uses traces but leaves the slide (Canvas)
     * 
     * SaveBlackb: Writes the current state of the canvas to a .pnm file (Canvas)
     * 
     * Quit: Ends the program (Canvas, even only main takes care of it)
     * 
     * FastForward: Goes 10 slides further from the current one (PDFSlide)
     * 
     * FastBackwards: Goes 10 slides before the current one (PDFSlide)
     * 
     * ToFirstSlide: Goes to the first slide (PDFSLide)
     * 
     * ToLastSlide: Goes to the last slide (PDFSlide)
     * 
     * NoCommand: Special mark to account for press of unassigned keys. Nothing is done (Canvas)
     * 
    */
    enum Commands 
    { DrawErase, LineCharac, Next, Previous, EraseAll, EraseSlide, EraseBlackb, SaveBlackb, Quit, FastForward, FastBackwards, ToFirstSlide, ToLastSlide, NoCommand };
    
    /**
     * The command that appears as the first entry of the menu
     */
    static const Commands FirstMenuCommand = DrawErase;
    
    /**
     * The command that appears as the last entry of the menu
     * 
     * These constants are used to go from coordinates of mouse click to the command, assuming that all entry menus will have the same width.
     */
    static const Commands LastMenuCommand = Quit;
    
    /**
     * Constructor. It does not need any argument. Everything is read from the configuration file, whose name is fixed.
     * 
     */
    Config();
    
    /**
     * Destructor. Nothing to do in it, since this class does not use dynamic allocation. It is nevertheless necessary to avoid warnings because the -Winline flag.
     */
    ~Config() {};
    
    /**
     * Test to know if the program must be opened in a window or use the full screen,
     * \return true for window, false for full screen mode.
     */
    bool GetInWin(void) { return in_window; };
    
    /**
     * Gets the horizontal resolution in pixels of the window or screen
     * \return X-resolution in pixels
     */
    int GetXres(void) { return scw; };
    
    /**
     * Gets the vertical resolution in pixels of the window or screen
     * \return Y-resolution in pixels
     */
    int GetYres(void) { return sch; };
    
    /** 
     * Stores the horizontal and vertical resolution of the window
     * 
     * This function will be called only when the user has requested full-screen mode and the resolution is obtained from the system
     * \param w Horizontal size
     * \param h Vertical size
     */
    void SetRes(int w,int h);
    
    /**
     * Gets the file absolute path of the chosen font that the drawing part will need
     * \return Absolute path of the chosen TTF font,
     */
    std::string GetFont(void) { return (font_dir+font_name); };
    
    /**
     * Gets the size in pixels of the spot to erase pixels under it
     * \return The size of the eraser, either as radius (for cicular eraser) or as side (for square eraser)
     */
    int GetEraserSize(void) { return eraser_size; };
    
    /**
     * Gets the size of the font to write the menu items and messages
     * \return Size of the font, in points.
     */
    int GetFontsize(void) { return font_size; };
    
    /**
     * Checks if teh config file has asked for showing a banner at program start or not
     * \return true if a banner is to be shown, false if not
     */
    bool GetShowSplash(void) { return show_splash; };
    
    /**
     * Gets the name of the PDF file with the initial banner 
     * \return Absolute path of the PDF banner file
     */
    std::string GetSplashFile(void) { return splash_file; };
    
    /**
     * This function returns a command to be executed, according to the key the user has pressed. If the key is associated to one element of the menu, or is one of the predefined ones, it decides which one. If not, it is ignored and NoCommand is returned.
     * \param key The pressed key
     * \param for_canvas Parameter to return by reference if the command is to be sent to a Canvas object (returns with true) or to a PDFSlide object (returns with false)
     * \return A command, as given by the enumeration Commands in this class.
     */
    Commands InterpretKey(int key,bool &for_canvas);
    
    /** 
     * Gets the height in pixels of the upper menu. Its width is that of the screen or window.
     * \return Height of the menu, in pixels
     */
    int GetMenuHeight(void) { return menu_height; };
    
    /**
     * Gets the message to be shown when a blackboard is saved to a file. This message is read from the language configuration file
     * \return The message to be shown when saving a blackboard to a .ppm file
     */
    std::string GetSaveMessage() { return sitems[0]; };
    
    /** 
     * Gets the message to be shown when a blackboard could not be saved to a file (probably, because of permissions problems). This message is read from the language configuration file
     * \return The message to be shown when the program has been unable to save a blackboard to a .ppm file
     */
    std::string GetErrorSaveMessage() { return sitems[1]; };
    
    /** 
     * Gets a vector of strings with the texts of each menu item, as read from the language configuration file
     * \return An ordered vector of menu items
     */
    const std::vector<std::string> &GetMenuItems() { return mitems; };
    
 private:
    static const int MaxMenuHeight = 20;
    static const int Attend=-1;
    
    ConfigParams ResolveParam(const std::string &l);
    std::string SearchConfigFile(void);
    void SearchLangMenuFile(void);
    void ParseConfigFile(void);
    bool LineHasInfo(const std::string &l,std::string cfile,int nline);
    ValidityValues ValidContent(const std::string &p,const std::string &v);
    void ReadLangSection();

    bool show_splash;
    std::string splash_file;
    
    bool in_window;
    int scw,sch;
    
    int menu_height;
    
    std::string font_dir;
    std::string font_name;
    unsigned font_size;
    
    unsigned eraser_size;
    EraserShapes eraser_shape;
    
    std::string lang_file;
       
    std::vector<std::string> mitems;
    std::vector<std::string> sitems;
    std::vector< std::pair<int,int> > accelerator_codes;
};

#endif
