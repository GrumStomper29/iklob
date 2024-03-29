if(COMMAND cmake_policy)
    # Allow VERSION in the project() statement.
    cmake_policy(SET CMP0048 NEW)
endif()

set(GTE_VERSION_MAJOR 6)
set(GTE_VERSION_MINOR 5)

project(Mathematics VERSION ${GTE_VERSION_MAJOR}.${GTE_VERSION_MINOR})

cmake_minimum_required(VERSION 3.8)
add_subdirectory(ApproximateBezierCurveByArcs)
add_subdirectory(ApproximateEllipsesByArcs)
add_subdirectory(BSplineCurveFitter)
add_subdirectory(BSplineCurveReduction)
add_subdirectory(BSplineSurfaceFitter)
add_subdirectory(FitCone)
add_subdirectory(FitConeByEllipseAndPoints)
add_subdirectory(FitCylinder)
add_subdirectory(FitTorus)
add_subdirectory(GeodesicEllipsoid)
add_subdirectory(GeodesicHeightField)
add_subdirectory(Interpolation2D)
add_subdirectory(NURBSCircle)
add_subdirectory(NURBSCurveExample)
add_subdirectory(NURBSSphere)
add_subdirectory(PartialSums)
add_subdirectory(PlaneEstimation)
add_subdirectory(RootFinding)
add_subdirectory(ShortestPath)
add_subdirectory(SymmetricEigensolver3x3)
add_subdirectory(ThinPlateSplines)
