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
#include "config.h"

Config::Config(void)
{
 // Fill default values
 in_window=true;
 scw=DefaultXRes;
 sch=DefaultYRes;
 font_dir=DefaultFontDir;
 font_name=DefaultFont;
 font_size=DefaultFontSize;
 eraser_size=DefaultEraserSize;
 eraser_shape=DefaultEraserShape;
 lang_file=std::string(DefaultGlobalConfigDir)+std::string(LangFileNameGlobal);

 SearchConfigFile();
 SearchLangMenuFile();
 ParseConfigFile();
 ReadLangSection();

}

std::string Config::SearchConfigFile(void)
{
 std::string cf1;

 char *h=getenv("HOME");
 cf1=std::string(h)+"/"+ConfigFileNameLocal;
 if (access(cf1.c_str(),R_OK)==0)
  return(cf1);

 std::string cf2=std::string(DefaultGlobalConfigDir)+std::string(ConfigFileNameGlobal);
 if (access(cf2.c_str(),R_OK)==0)
 {
  std::string copy_command="cp "+cf2+" "+cf1;
  if (system(copy_command.c_str())==0)
  {
   std::cerr << std::endl << std::endl;
   std::cerr << "   *************   INFO from vbb   *****************\n";
   std::cerr << "   The global configuration file " << cf2 << std::endl;
   std::cerr << "   has been copied to the local one " << cf1 << ".\n";
   std::cerr << "   You can customize this local file as you wish or need.\n";
   std::cerr << "   *************************************************\n\n";
   return(cf1);
  }
  else
  {
   std::cerr << "WARNING from vbb: the global configuration file " << cf2 << " could not be copied to the local one, " << cf1 << ".\n";
   std::cerr << "It might be a permissions problem or a wrong value for the $HOME environment variable.\n";
   std::cerr << "Global file " << cf2 << " will be used, but you will not be able to customize it without root privileges.\n";
   return(cf2);
  }
 }
 
 std::cerr << "Neither configuration file '" << cf1 << "' nor file '" << cf2 << " can be opened. Check existance and permissions. Exiting.\n";
 exit(1);
}

void Config::SearchLangMenuFile(void)
{
 std::string mf1;
 
 char *h=getenv("HOME");
 mf1=std::string(h)+"/"+LangFileNameLocal;
 if (access(mf1.c_str(),R_OK)==0)
 {
  lang_file=mf1;
  return;
 }

 std::string mf2=std::string(DefaultGlobalConfigDir)+std::string(LangFileNameGlobal);
 if (access(mf2.c_str(),R_OK)==0)
 {
  std::string copy_command="cp "+mf2+" "+mf1;
  if (system(copy_command.c_str())==0)
  {
   std::cerr << "   *************   INFO from vbb   *****************\n";
   std::cerr << "   The language menu configuration file " << mf2 << std::endl;
   std::cerr << "   has been copied to the local one " << mf1 << ".\n";
   std::cerr << "   You can customize this local file as you wish or need.\n";
   std::cerr << "   *************************************************\n\n";
   lang_file=mf1;
   return;
  }
  else
  {
   std::cerr << "WARNING from vbb: the global language menu configuration file " << mf2 << " could not be copied to the local one, " << mf1 << ".\n";
   std::cerr << "It might be a permissions problem or a wrong value for the $HOME environment variable.\n";
   std::cerr << "Global file " << mf2 << " will be used, but you will not be able to customize it without root privileges.\n";
   lang_file=mf2;
   return;
  }
 }
 
 std::cerr << "Neither language configuration file '" << mf1 << "' nor file '" << mf2 << " can be opened. Check existance and permissions. Exiting.\n";
 exit(1);
}

bool Config::LineHasInfo(const std::string &l, std::string cfile,int nline)
{
 // A comment-line must start with '#' and has no info.
 if (l.find("#")!=std::string::npos)
 {
  if (l[0]=='#')
   return false;
  else
  {
   std::cerr << "Error in line " << nline << " of configuration file '" << cfile << "'.\n";
   std::cerr << "Comment character '#' is allowed only at the beginning of a line.\n";
   exit(1); 
  }
 }
 // Otherwise, an empty line (only composed by blank spaces, tabs and end-of-line characters) has no info.
 return (l.find_first_not_of(" \t\r\n") != std::string::npos);
}

Config::ConfigParams Config::ResolveParam(const std::string &l)
{
 std::map<std::string, ConfigParams>::const_iterator itr = ParamStrings.find(l);
 if ( itr != ParamStrings.end() )
 {
        return (*itr).second;
 }
 return UnknownParam;
}

void Config::ParseConfigFile(void)
{
 std::string config_file=SearchConfigFile();
 
 std::ifstream f(config_file.c_str());
 if (!f.is_open())
 {
  std::cerr << "Error: config file " << config_file << " cannot be opened (even we cheked for permissions before...).\n";
  exit(1);
 }

 int nline=0;
 while (f.eof()==false)
 {
  std::string line;
  getline(f,line);
  if (LineHasInfo(line,config_file,nline))
  {
   if (line.find(":")==std::string::npos)
   {
    std::cerr << "Error parsing line " << nline << " of configuration file '" << config_file << "'.\n";
    std::cerr << "It should be of the form\nconfig_parameter: value\n(there is no ':' character).\n";
   }
   std::string param="",val="";
   bool colon_found=false;
   for (std::string::iterator it=line.begin(); it!=line.end(); ++it)
   {
    if ((*it!=' ') && (*it!='\t'))
    {
     if (*it==':')
      colon_found=true;
     else
     {
      if (!colon_found)
       param.push_back(*it);
      else
       val.push_back(*it);
     }
    }
   }
   ValidityValues vc=ValidContent(param,val);
   if (vc!=ValidPair)
   {
    std::cerr << ((vc==InvalidParam) ? "Invalid parameter " : "Invalid value ");
    std::cerr << "in line " << nline << " ('" << line << "') of configuration file '" << config_file << "'. Exiting.\n";
    exit(1);
   }
  }
  nline++;
 }
 //cerr << "Configuration file " << config_file << " with " << nline << " lines correctly read.\n";
 f.close();
}

Config::ValidityValues Config::ValidContent(const std::string &p,const std::string &v)
{
 switch (ResolveParam(p))
 {
  case OpenInWindow: 
	{
	 if (v=="yes")
	 {
	  in_window=true;
	  return ValidPair;
	 }
	 if (v=="no")
	 {
	  in_window=false;
          scw=sch=Attend;
	  return ValidPair;
	 }
	 return InvalidValue;
	 break;
	}
  case XRes:
	{
         if (!in_window)
         {
          scw=Attend;
          return ValidPair;
         }
	 char *p;
	 long converted = strtol(v.c_str(),&p,10);
	 if (*p == '\0') 
	 {
	  scw = int(converted);
	  return ValidPair;
	 }
	 else
	  return InvalidValue;
	 break;
	}
  case YRes:
	{
         if (!in_window)
         {
          sch=Attend;
          return ValidPair;
         }
	 char *p;
	 long converted = strtol(v.c_str(),&p,10);
	 if (*p == '\0') 
	 {
	  sch = int(converted);
	  menu_height = ((sch/10)>MaxMenuHeight) ? MaxMenuHeight : (sch/10);
	  return ValidPair;
	 }
	 else
	  return InvalidValue;
	 break;
	}
  case EraserSize:
	{
	 char *p;
	 long converted = strtol(v.c_str(),&p,10);
	 if (*p == '\0') 
	 {
	  eraser_size = int(converted);
	  return ValidPair;
	 }
	 else
	  return InvalidValue;
	 break;
	}
  case EraserShape:
	{
	 if (v=="square")
	 {
	  eraser_shape=EraserShapeSquare;
	  return ValidPair;
	 }
	 if (v=="circle")
	 {
	  eraser_shape=EraserShapeCircle;
	  return ValidPair;
	 }
	 return InvalidValue;
	 break;
	}
 // Now, check the parameters that involve files.
 // Particularly, check if files exist.
  case FontDir:
	{
	 font_dir=v;
 	 if (font_dir.find_last_of('/')!=font_dir.size()-1)
	  font_dir=font_dir+std::string("/");
	 return ValidPair;
	 break;
	}
  case FontName:
	{
	 font_name=v;
	 std::string fn=font_dir+font_name;
	 if (access( fn.c_str(), R_OK ) != 0)
	 {
	  std::cerr << "Error: cannot open font file '" << fn << "' for reading. Check existance and permissions.\n";
   	  return InvalidValue;
  	 }
  	 return ValidPair;
	 break;
 	}
  case FontSize:
	{
	 char *p;
	 long converted = strtol(v.c_str(),&p,10);
	 if (*p == '\0') 
	 {
	  font_size = int(converted);
	  return ValidPair;
	 }
	 else
	  return InvalidValue;
	 break;
	}
  case LangFile:
	{
	 std::string nf1=std::string(getenv("HOME"))+"/."+v;
	 if (access( nf1.c_str(), R_OK ) != 0)
	 {
	  std::string nf2="/etc/"+v;
	  if (access( nf1.c_str(), R_OK ) != 0)
	  {
	   std::cerr << "Error trying to open language file '" << nf1 << "' or '" << nf2 << "'. They don't exist or cannot be opened.\n";
	   return InvalidValue;
	  }
	  lang_file=nf2;
  	 }
  	 else
	  lang_file=nf1;
	 return ValidPair;
	 break;
	}
  case SplashFile:
	{
	 splash_file=v;
	 if (v=="None")
	 {
	  show_splash=false;
	  return ValidPair;
     }
     show_splash=true;
	 if ( access(v.c_str(),F_OK) == 0 ) 
	  splash_file=v;
	 else
     {
	  std::cerr << "Warning: cannot open splash file " << v << std::endl;
	  std::cerr << "The default splash screen will be shown.\n";
      splash_file=std::string(DefaultGlobalConfigDir)+std::string(DefaultSplashFile);
     }
	 return ValidPair;
	 break;
 	}
  case UnknownParam: return InvalidParam; break;
  default: // We should never have arrived here, but..
	  return InvalidParam; break;
 }
 // Special case: the user has asked for full screen. In this case, the dimensions given in the .cfg file should be ignored.
 // But we cannot yet fix it to the real screen size, since SDL is not yet initalized (it will be in the canvas constructor).
 // Therefore, we mark width and height with the special mark ATTEND to wait proper initalization.
 if (!in_window)
 {
  scw=sch=Attend;
  std::cerr << "ATTEND RES...\n";
 }
}

void Config::SetRes(int w,int h)
{ 
 scw=w;
 sch=h;
 menu_height = ((sch/10)>MaxMenuHeight) ? MaxMenuHeight : (sch/10);
}

void Config::ReadLangSection()
{
 std::ifstream f(lang_file.c_str());
 if (!f.is_open())
 {
  std::cerr << "Error trying to open language file '" << lang_file << "'. Exiting.\n";
  exit(1);
 }
 
 std::string item;
 do
 {
  getline(f,item);
 }
 while (!f.eof() && item!="MENU");
 if (f.eof())
 {
  std::cerr << "Error reading language file " << lang_file << ". No MENU section found (or it is after SAVE section, which is not allowed).\n";
  exit(1);
 }
 do
 {
  getline(f,item);
 }
 while (!f.eof() && item!="SAVE");
 if (f.eof())
 {
  std::cerr << "Error reading language file " << lang_file << ". No SAVE section found (or it is before MENU section, which is not allowed).\n";
  exit(1);
 }

 f.seekg(0,std::ios::beg);
 int nline=0;
 do
 {
  getline(f,item);
  nline++;
 }
 while (!f.eof() && item!="MENU");
 do
 {
  getline(f,item);
  nline++;
  if (LineHasInfo(item,lang_file,nline) && item!="SAVE")
  {
   mitems.push_back(item);
   unsigned int p=item.find_first_of("(");
   std::pair<int,int> codes;
   codes.first=toupper(int(item[p+1]));
   codes.second=tolower(int(item[p+1]));
   unsigned i=0;
   while (i<accelerator_codes.size() && accelerator_codes[i].first!=codes.first)
    i++;
   if (i<accelerator_codes.size())
   {
    std::cerr << "Error reading language file " << lang_file << ". Accelerator key " << item[p+1] << " has been used more than once. Exiting.\n";
    exit(1);
   }
   accelerator_codes.push_back(codes);
  }
 }
 while (!f.eof() && item!="SAVE");

 do
 {
  getline(f,item);
  if (!f.eof())
  {
   if (item.find("%s")==std::string::npos)
   {
    std::cerr << "Error reading language file " << lang_file << ". A string in the SAVE section does not contain '%s' to refer to the file to save.\n";
    exit(1);
   }
   sitems.push_back(item);
  }
 }
 while (!f.eof());
 if (sitems.size()<2)
 {
  std::cerr << "Error reading language file " << lang_file << ". Less than two strings in the SAVE section.\n";
  exit(1);
 }
 f.close();
}

Config::Commands Config::InterpretKey(int key,bool &to_canvas)
{
 // If the key is in the table, it corresponds to one of the visible menu commands. The order of commands in the table is as they appear in the upper menu.
 unsigned i=0;
 while (i<accelerator_codes.size() && key!=accelerator_codes[i].first && key!=accelerator_codes[i].second)
  i++;
 if (i<accelerator_codes.size())
 {
  to_canvas = ((i<=LastMenuCommand) && (i!=Next) && (i!=Previous));
  return( Commands(i) );
 }

 // If the key is not in the table, it is one of those which are accessible only as key presses.
 // All of these are to be sent to the PDFSlide, not to the Canvas
 to_canvas=false;
 switch (key)
 {
     case SDLK_RIGHT: return(Next); break;
     case SDLK_LEFT: return(Previous); break;
     case SDLK_PAGEUP: return(ToLastSlide); break;
     case SDLK_UP: return(FastForward); break;
     case SDLK_PAGEDOWN: return(ToFirstSlide); break;
     case SDLK_DOWN: return(FastBackwards); break;
     default: break;
 }
 return(NoCommand);
}

