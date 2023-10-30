#!/usr/bin/python3
import sys
import numpy as np
import pyvista as pv

points = np.loadtxt(sys.argv[1],delimiter=' ')
cloud = pv.PolyData(points)

volume = cloud.delaunay_3d(alpha=int(sys.argv[2]))
shell = volume.extract_geometry()
shell.plot()
