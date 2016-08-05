-- Native file dialog premake5 script
--
-- This can be ran directly, but commonly, it is not.
-- The product of this script is checked in to source control,
-- so you don't need to worry about the extra step when building.

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

    filter "system:macosx"
      language "C"
      files {root_dir.."src/nfd_cocoa.m"}

    filter "system:linux"
      language "C"
      files {root_dir.."src/nfd_gtk.c"}
      buildoptions {"`pkg-config --cflags gtk+-3.0`"}

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

    filter {"configurations:Release", "system:linux"}
      linkoptions {"-lnfd `pkg-config --libs gtk+-3.0`"}

    filter {"system:macosx"}
      links {"Foundation.framework", "AppKit.framework"}
      
    filter {"configurations:Debug", "system:linux"}
      linkoptions {"-lnfd_d `pkg-config --libs gtk+-3.0`"}
   
end
      
make_test("test_opendialog")
make_test("test_opendialogmultiple")
make_test("test_savedialog")

newaction
{
   trigger = "dist",
   description = "Create distributable premake dirs (maintainer only)",
   execute = function()
      types_to_create =
         {
            "vs2010",
            "xcode4",
            "gmake"
         }

      for i,v in ipairs(types_to_create) do
         local premake_file = "./"..v.."/premake5.lua"
         os.mkdir(v)
         os.execute("cp premake5.lua "..v)
         os.execute("premake5 --file="..premake_file.." "..v)
         os.execute("rm "..premake_file)
      end
      
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
            "xcode4"
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
