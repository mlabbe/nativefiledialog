-- Native file dialog premake5 script
--
-- This can be ran directly, but commonly, it is only run
-- by package maintainers.
--
-- IMPORTANT NOTE: premake5 alpha 9 does not handle this script
-- properly.  Build premake5 from Github master, or, presumably,
-- use alpha 10 in the future.


newoption {
   trigger     = "linux_backend",
   value       = "B",
   description = "Choose a dialog backend for linux",
   allowed = {
      { "gtk3", "GTK 3 - link to gtk3 directly" },      
      { "zenity", "Zenity - generate dialogs on the end users machine with zenity" }
   }
}

if not _OPTIONS["linux_backend"] then
   _OPTIONS["linux_backend"] = "gtk3"
end

workspace "NativeFileDialog"
  -- these dir specifications assume the generated files have been moved
  -- into a subdirectory.  ex: $root/build/makefile
  local root_dir = path.join(path.getdirectory(_SCRIPT),"../../")
  local build_dir = path.join(root_dir,"build/")
  configurations { "Release", "Debug" }
  platforms {"x64", "x86"}

  objdir(path.join(build_dir, "obj/"))

  -- architecture filters
  filter "configurations:x86"
    architecture "x86"
  
  filter "configurations:x64"
    architecture "x86_64"

  -- debug/release filters
  filter "configurations:Debug"
    defines {"DEBUG"}
    symbols "On"
    targetsuffix "_d"

  filter "configurations:Release"
    defines {"NDEBUG"}
    optimize "On"

  project "nfd"
    kind "StaticLib"

    -- common files
    files {root_dir.."src/*.h",
           root_dir.."src/include/*.h",
           root_dir.."src/nfd_common.c",
    }

    includedirs {root_dir.."src/include/"}
    targetdir(build_dir.."/lib/%{cfg.buildcfg}/%{cfg.platform}")

    -- system build filters
    filter "system:windows"
      language "C++"
      files {root_dir.."src/nfd_win.cpp"}

    filter {"action:gmake or action:xcode4"}
      buildoptions {"-fno-exceptions"}

    filter "system:macosx"
      language "C"
      files {root_dir.."src/nfd_cocoa.m"}



    filter {"system:linux", "options:linux_backend=gtk3"}
      language "C"
      files {root_dir.."src/nfd_gtk.c"}
      buildoptions {"`pkg-config --cflags gtk+-3.0`"}
    filter {"system:linux", "options:linux_backend=zenity"}
      language "C"
      files {root_dir.."src/nfd_zenity.c"}


    -- visual studio filters
    filter "action:vs*"
      defines { "_CRT_SECURE_NO_WARNINGS" }      

local make_test = function(name)
  project(name)
    kind "ConsoleApp"
    language "C"
    dependson {"nfd"}
    targetdir(build_dir.."/bin")
    files {root_dir.."test/"..name..".c"}
    includedirs {root_dir.."src/include/"}


    filter {"configurations:Debug", "architecture:x86_64"}
      links {"nfd_d"}
      libdirs {build_dir.."/lib/Debug/x64"}

    filter {"configurations:Debug", "architecture:x86"}
      links {"nfd_d"}
      libdirs {build_dir.."/lib/Debug/x86"}

    filter {"configurations:Release", "architecture:x86_64"}
      links {"nfd"}
      libdirs {build_dir.."/lib/Release/x64"}

    filter {"configurations:Release", "architecture:x86"}
      links {"nfd"}
      libdirs {build_dir.."/lib/Release/x86"}

    filter {"configurations:Debug"}
      targetsuffix "_d"

    filter {"configurations:Release", "system:linux", "options:linux_backend=gtk3"}
      linkoptions {"-lnfd `pkg-config --libs gtk+-3.0`"}
    filter {"configurations:Release", "system:linux", "options:linux_backend=zenity"}
      linkoptions {"-lnfd"}

    filter {"system:macosx"}
      links {"Foundation.framework", "AppKit.framework"}
      
    filter {"configurations:Debug", "system:linux", "options:linux_backend=gtk3"}
      linkoptions {"-lnfd_d `pkg-config --libs gtk+-3.0`"}
    filter {"configurations:Debug", "system:linux", "options:linux_backend=zenity"}
      linkoptions {"-lnfd_d"}



    filter {"action:gmake", "system:windows"}
      links {"ole32", "uuid"}

end

make_test("test_pickfolder")
make_test("test_opendialog")
make_test("test_opendialogmultiple")
make_test("test_savedialog")
make_test("dialog")

newaction
{
   trigger = "dist",
   description = "Create distributable premake dirs (maintainer only)",
   execute = function()


      local premake_do_action = function(action,os_str,special,args)
         local premake_dir
         if special then
            if args['linux_backend'] ~= nil then
               premake_dir = "./"..action.."_"..os_str..'_zenity'
            else
               premake_dir = "./"..action.."_"..os_str
            end
         else
            premake_dir = "./"..action
         end
         local premake_path = premake_dir.."/premake5.lua"

         -- create an args str to pass along to premake
         arg_str = ''
         for arg, val in pairs(args) do
            arg_str = ' --'..arg..'='..val
         end

         os.execute("mkdir "..premake_dir)
         os.execute("cp premake5.lua "..premake_dir)
         os.execute("premake5 --os=" ..os_str..
                       " --file="..premake_path..
                       arg_str..
                       " "..action)
         os.execute("rm "..premake_path)
      end
      
      premake_do_action("vs2010", "windows", false,{})
      premake_do_action("xcode4", "macosx", false,{})
      premake_do_action("gmake", "linux", true,{})
      premake_do_action("gmake", "linux", true,{linux_backend='zenity'})
      premake_do_action("gmake", "macosx", true,{})
      premake_do_action("gmake", "windows", true,{})
   end
}

newaction
{
    trigger     = "clean",
    description = "Clean all build files and output",
    execute = function ()

        files_to_delete = 
        {
            "Makefile",
            "*.make",
            "*.txt",
            "*.7z",
            "*.zip",
            "*.tar.gz",
            "*.db",
            "*.opendb",
            "*.vcproj",
            "*.vcxproj",
            "*.vcxproj.user",
            "*.vcxproj.filters",
            "*.sln",
            "*~*"
        }

        directories_to_delete = 
        {
            "obj",
            "ipch",
            "bin",
            ".vs",
            "Debug",
            "Release",
            "release",
            "lib",
            "test",
            "makefiles",
            "gmake",
            "vs2010",
            "xcode4",
            "gmake_linux",
            "gmake_macosx",
            "gmake_windows"
        }

        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end

        if os.is "macosx" then
           os.execute("rm -rf *.xcodeproj")
           os.execute("rm -rf *.xcworkspace")
        end

        if not os.is "windows" then
            os.execute "find . -name .DS_Store -delete"
            for i,v in ipairs( files_to_delete ) do
              os.execute( "rm -f " .. v )
            end
        else
            for i,v in ipairs( files_to_delete ) do
              os.execute( "del /F /Q  " .. v )
            end
        end

    end
}
