#!lua

-- A solution contains projects, and defines the available configurations
solution "Engine"
   configurations { "Debug", "Release" }
   platforms { "native", "x64", "x32" }
   --startproject "Editor" -- Set this project as startup project
   location ( "./" ) -- Location of the solutions
         
    -- Project
    project "Engine"
      kind "ConsoleApp"
      language "C++"
      location ( "./Engine/" ) 
      targetdir ("./Engine/bin/")

      --buildoptions("-stdlib=libstdc++")
      buildoptions_cpp("-std=c++11")
      buildoptions_objcpp("-std=c++11")
      
      includedirs { 
           "./Engine/include", 
           "./Engine/dependencies",
      }
      
      -- INCLUDE FILES
      files { -- GLEW
        group = "GLEW", "./Engine/dependencies/glew/include/glew.c", 
        --"./Engine/dependencies/glew/include/GL/glew.h"
      }
      --files { -- tinyobjloader
      --  group = "tinyobj", "./Engine/dependencies/tinyobj/tiny_obj_loader.cc"
      --}
      
      files{ group = "include", "./Engine/include/**.h" } -- include filter and get the files
      files{ group = "src", "./Engine/src/**.cc", "./Engine/src/**.cpp" } -- src filter and get the files
      
	    -- only when compiling as library
      --defines { "GLEW_STATIC" }
	  
      configuration { "windows" }
         links {
          "opengl32"
         }
         files {  -- GLFW
            group = "GLFW", "./Engine/dependencies/GLFW/src/context.c", 
              "./Engine/dependencies/GLFW/src/init.c", 
              "./Engine/dependencies/GLFW/src/input.c",
              "./Engine/dependencies/GLFW/src/monitor.c",
              "./Engine/dependencies/GLFW/src/wgl_context.c",
              "./Engine/dependencies/GLFW/src/win32_init.c",
              "./Engine/dependencies/GLFW/src/win32_monitor.c",
              "./Engine/dependencies/GLFW/src/win32_time.c",
              "./Engine/dependencies/GLFW/src/win32_tls.c",
              "./Engine/dependencies/GLFW/src/win32_window.c",
              "./Engine/dependencies/GLFW/src/window.c",
              "./Engine/dependencies/GLFW/src/winmm_joystick.c",
            --"./Engine/dependencies/GLFW/include/GLFW/glfw3.h"
         }
         defines { "_GLFW_WIN32", "_GLFW_WGL", "_GLFW_USE_OPENGL" }
         
       configuration { "macosx" }
         links  {
           "Cocoa.framework", "OpenGL.framework", "IOKit.framework", "CoreVideo.framework",
         }
         linkoptions { "-framework Cocoa","-framework QuartzCore", "-framework OpenGL", "-framework OpenAL" }
          files {  -- GLFW
            group = "GLFW", "./Engine/dependencies/GLFW/src/context.c", 
              "./Engine/dependencies/GLFW/src/init.c", 
              "./Engine/dependencies/GLFW/src/input.c",
              "./Engine/dependencies/GLFW/src/monitor.c",
              "./Engine/dependencies/GLFW/src/nsgl_context.m",
              "./Engine/dependencies/GLFW/src/cocoa_init.m",
              "./Engine/dependencies/GLFW/src/cocoa_monitor.m",
              "./Engine/dependencies/GLFW/src/mach_time.c",
              "./Engine/dependencies/GLFW/src/posix_tls.c",
              "./Engine/dependencies/GLFW/src/cocoa_window.m",
              "./Engine/dependencies/GLFW/src/window.c",
              "./Engine/dependencies/GLFW/src/iokit_joystick.m",
            --"./Engine/dependencies/GLFW/include/GLFW/glfw3.h"
         }
         defines { "_GLFW_COCOA", "_GLFW_NSGL", "_GLFW_USE_OPENGL" }
         
       configuration { "linux" }
          files {  -- GLFW
            group = "GLFW", "./Engine/dependencies/GLFW/src/context.c", 
              "./Engine/dependencies/GLFW/src/init.c", 
              "./Engine/dependencies/GLFW/src/input.c",
              "./Engine/dependencies/GLFW/src/monitor.c",
              "./Engine/dependencies/GLFW/src/glx_context.c",
              "./Engine/dependencies/GLFW/src/x11_init.c",
              "./Engine/dependencies/GLFW/src/x11_monitor.c",
              "./Engine/dependencies/GLFW/src/posix_time.c",
              "./Engine/dependencies/GLFW/src/posix_tls.c",
              "./Engine/dependencies/GLFW/src/x11_window.c",
              "./Engine/dependencies/GLFW/src/window.c",
              "./Engine/dependencies/GLFW/src/linux_joystick.c",
            --"./Engine/dependencies/GLFW/include/GLFW/glfw3.h"
         }
         defines { "_GLFW_X11", "_GLFW_GLX", "_GLFW_USE_OPENGL" }

       configuration "debug"
         defines { "DEBUG" }
         flags { "Symbols", "ExtraWarnings"}

       configuration "release"
         defines { "NDEBUG" }
         flags { "Optimize", "ExtraWarnings" }