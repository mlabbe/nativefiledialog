-- Native file dialog premake5 script
--
-- This can be ran directly, but commonly, it is not.
-- The product of this script is checked in to source control,
-- so you don't need to worry about the extra step when building.

workspace "NativeFileDialog"
  local root_dir = path.join(path.getdirectory(_SCRIPT),"..".."/")
  configurations { "Debug", "Release" }
  platforms {"x86", "x64"}

  -- architecture filters
  filter "configurations:x86"
    architecture "x86"
  
  filter "configurations:x64"
    architecture "x86_64"

  -- debug/release filters
  filter "configurations:Debug"
    defines {"DEBUG"}
    flags {"Symbols"}
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
    targetdir(root_dir.."build/lib/%{cfg.buildcfg}/%{cfg.platform}")

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
    targetdir(root_dir.."build/test")
    files {root_dir.."test/"..name..".c"}
    includedirs {root_dir.."src/include/"}


    filter {"configurations:Debug", "architecture:x86_64"}
      links {"nfd_d"}
      libdirs {root_dir.."build/lib/Debug/x64"}

    filter {"configurations:Debug", "architecture:x86"}
      links {"nfd_d"}
      libdirs {root_dir.."build/lib/Debug/x86"}

    filter {"configurations:Release", "architecture:x86_64"}
      links {"nfd"}
      libdirs {root_dir.."build/lib/Release/x64"}

    filter {"configurations:Release", "architecture:x86"}
      links {"nfd"}
      libdirs {root_dir.."build/lib/Release/x86"}

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

