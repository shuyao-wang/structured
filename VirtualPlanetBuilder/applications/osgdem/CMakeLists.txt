#this file is automatically generated 

INCLUDE_DIRECTORIES(${GDAL_INCLUDE_DIR} ${OSG_INCLUDE_DIRS} )
SET(TARGET_LIBRARIES_VARS GDAL_LIBRARY OSG_LIBRARY OSGVIEWER_LIBRARY )

SET(TARGET_SRC osgdem.cpp )

#### end var setup  ###
SETUP_APPLICATION(osgdem)
