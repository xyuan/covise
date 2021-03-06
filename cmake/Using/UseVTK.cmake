MACRO(USE_VTK)
  IF ((NOT VTK_FOUND) AND (${ARGC} LESS 1))
    USING_MESSAGE("Skipping because of missing VTK")
    RETURN()
  ENDIF((NOT VTK_FOUND) AND (${ARGC} LESS 1))
  IF(NOT VTK_USED AND VTK_FOUND)
    SET(VTK_USED TRUE)
    IF(MSVC)
      SET(VTK_LIBRARIESIO debug vtkFilteringd optimized vtkFiltering debug vtkDICOMParserd optimized vtkDICOMParser debug vtkNetCDFd optimized vtkNetCDF debug vtkmetaiod optimized vtkmetaio debug vtksqlited optimized vtksqlite debug vtkpngd optimized vtkpng debug vtkzlibd optimized vtkzlib debug vtkjpegd optimized vtkjpeg debug vtktiffd optimized vtktiff debug vtkexpatd optimized vtkexpat debug vtksysd optimized vtksys general vfw32)
      SET(VTK_LIBRARIES debug vtkRenderingd optimized vtkRendering debug vtkGraphicsd optimized vtkGraphics debug vtkImagingd optimized vtkImaging debug vtkIOd optimized vtkIO debug vtkftgld optimized vtkftgl debug vtkfreetyped optimized vtkfreetype general opengl32)
      SET(VTK_LIBRARIESFILTERING debug vtkCommond optimized vtkCommon)
      SET(VTK_LIBRARIESCOMMON debug vtksysd optimized vtksys)
    
      LINK_DIRECTORIES(${VTK_LIBRARY_DIRS})
      INCLUDE_DIRECTORIES(${VTK_INCLUDE_DIRS})
      ADD_DEFINITIONS(-DHAVE_VTK)
      SET(EXTRA_LIBS ${EXTRA_LIBS} ${VTK_LIBRARIES} ${VTK_LIBRARIESIO} ${VTK_LIBRARIESFILTERING} ${VTK_LIBRARIESCOMMON})
    ELSE(MSVC)
      add_definitions(-DHAVE_VTK)
      include_directories(${VTK_INCLUDE_DIRS})
      set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS ${VTK_DEFINITIONS})
      IF(MINGW)
          SET(EXTRA_LIBS ${EXTRA_LIBS} vtkCommon vtkGraphics vtkIO vtkFiltering)
      ELSE(MINGW)
         set(EXTRA_LIBS ${EXTRA_LIBS} ${VTK_LIBRARIES})
      ENDIF(MINGW)
      if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
         SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
      elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
         SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated")
      endif()
    ENDIF(MSVC)
  ENDIF()
ENDMACRO(USE_VTK)

