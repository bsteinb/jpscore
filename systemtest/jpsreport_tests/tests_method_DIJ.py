#!/usr/bin/env python3
# ---------
# Basic test setup:
# Pedestrians are moving in a 6x6 grid in x-direction with v=1 m/s
# They have a fixed horizontal and vertical distance (ped_distance)
# Due to the alignment the inner voronoi cells should be squares
# The inner voronoi cells should lay entirely in the specified square measurement area for frame 109 (after a travelled distance of 9m)
# ---------

import os
from sys import argv, path
import logging
import math

path.append(os.path.dirname(path[0]))  # source helper file
from utils import SUCCESS, FAILURE
import numpy as np


# ---------
# Test of classical voronoi output for Method D, I and J
# Real voronoi density can be calculated by 1 person / (ped_distance^2)
# Real velocity is 1.0 m/s for all frames (since cut off is false parts of the outer voronoi cells lay in the measurement area)
# ---------

def test_classical_voronoi(trajfile, ped_distance):
    jpsreport_result_file = os.path.join('./Output',
                                         'Fundamental_Diagram',
                                         'Classical_Voronoi',
                                         'rho_v_Voronoi_%s_id_1.dat' % trajfile
                                         )

    if not os.path.exists(jpsreport_result_file):
        logging.critical("jpsreport did not output results correctly.")
        exit(FAILURE)

    # set test configuration
    # real density can be calculated based on the square/distance to each other
    real_density = 1 / (ped_distance ** 2)
    # real velocity must remain the same
    real_velocity = 1
    # accepted error
    acceptance_range = 0.001
    # frame when inner voronoi cells of 16 people are entirely in the measurement area
    frame = 109

    jpsreport_data = np.loadtxt(jpsreport_result_file)

    # check density for the specified frame
    jpsreport_density = jpsreport_data[jpsreport_data[:, 0] == frame][0, 1]

    if abs(jpsreport_density - real_density) < acceptance_range:
        logging.info("density in measurement area should be OK.")
    else:
        logging.critical(
            "density value in measurement area did not match result. Got {}. Expected {}".format(jpsreport_density,
                                                                                                 real_density))
        exit(FAILURE)

    # check velocity for all frames
    jpsreport_velocity = jpsreport_data[:, 2]
    real_velocity_array = np.ones(np.size(jpsreport_velocity)) * real_velocity

    if np.all(np.abs(jpsreport_velocity - real_velocity_array) < acceptance_range):
        logging.info("velocity calculation should be OK.")
    else:
        logging.critical(
            "velocity values did not match result. Got {}. Expected {}".format(jpsreport_velocity, real_velocity_array))
        exit(FAILURE)


# ---------
# Test of IFD output for Method D, I and J
# IDs of pedestrians that are in the measurement area for frame 109 are checked
# Their individual density must be 1 person / (ped_distance^2)
# ---------
def test_IFD(method, trajfile, ped_distance):
    jpsreport_result_file = os.path.join('./Output',
                                         'Fundamental_Diagram',
                                         'IndividualFD',
                                         'IFD_%s_%s_id_1.dat' % (method, trajfile)
                                         )

    if not os.path.exists(jpsreport_result_file):
        logging.critical("jpsreport did not output results correctly.")
        exit(FAILURE)

    # set test configuration
    # real density can be calculated based on the square/distance to each other
    real_density = 1 / (ped_distance ** 2)
    # accepted error
    acceptance_range = 0.001
    # frame when inner voronoi cells of 16 people are entirely in the measurement area
    frame = 109
    # ids of pedestrians whose voronoi cells are entireley in the measurement for the upper frame
    ped_IDs = np.array([8.0, 9.0, 10.0, 11.0, 14.0, 15.0, 16.0, 17.0, 20.0, 21.0, 22.0, 23.0, 26.0, 27.0, 28.0, 29.0])

    jpsreport_data = np.loadtxt(jpsreport_result_file, usecols=(0, 1, 2, 3, 4, 5))

    # get ids that are in the measurement area for the specified frame
    jpsreport_IDs = jpsreport_data[jpsreport_data[:, 0] == frame][:, 1]
    jpsreport_IDs_sorted = np.sort(jpsreport_IDs)
    real_ped_IDs_sorted = np.sort(ped_IDs)

    if np.array_equal(jpsreport_IDs_sorted, real_ped_IDs_sorted):
        logging.info("pedestrians in measurement area should be OK.")
    else:
        logging.critical(
            "IDs of pedestrians in measurement area did not match result. Got {}. Expected {}".format(
                jpsreport_IDs_sorted,
                real_ped_IDs_sorted))
        exit(FAILURE)

    # check density for for pedestrians in measurement area for specified frame
    jpsreport_density = jpsreport_data[jpsreport_data[:, 0] == frame][:, 5]
    real_density_array = np.ones(np.size(jpsreport_density)) * real_density

    if np.all(np.abs(jpsreport_density - real_density_array) < acceptance_range):
        logging.info("density calculation should be OK.")
    else:
        logging.critical(
            "density values did not match result. Got {}. Expected {}".format(jpsreport_density, real_density_array))
        exit(FAILURE)


# ---------
# Test of cut off option for Method D, I and J
# Densities of pedestrians that are in the measurement area for frame 109 are checked
# Cut off voronoi cells are approximated by circles
# If the area of the circle is smaller than the area of the square, the cut off option has an effect on the inner voronoi cells
# If the area of the circle is larger than the area of the square, the cut off option has no effect on the inner voronoi cells
# Both scenarios can be tested with this function by setting `cut_off_has_effect`
# ---------
def test_cut_off(method, trajfile, ped_distance, cut_off_has_effect=True):
    jpsreport_result_file = os.path.join('./Output',
                                         'Fundamental_Diagram',
                                         'IndividualFD',
                                         'IFD_%s_%s_id_1.dat' % (method, trajfile)
                                         )

    if not os.path.exists(jpsreport_result_file):
        logging.critical("jpsreport did not output results correctly.")
        exit(FAILURE)

    # set test configuration
    # real density is based on the square for circumcircle or based on the circle for incircle
    if cut_off_has_effect:
        # cut off is applied: density is higher than for square voronoi cell.
        real_density = 1 / (math.pi * (ped_distance / 2) ** 2)
    else:
        # no cut off is applied: density remains the same
        real_density = 1 / (ped_distance ** 2)

    # accepted error
    acceptance_range = 0.001
    # frame when inner voronoi cells of 16 people are entirely in the measurement area
    frame = 109

    jpsreport_data = np.loadtxt(jpsreport_result_file, usecols=(0, 1, 2, 3, 4, 5))

    # check density for pedestrians in measurement area for specified frame
    jpsreport_density = jpsreport_data[jpsreport_data[:, 0] == frame][:, 5]
    real_density_array = np.ones(np.size(jpsreport_density)) * real_density  #

    if np.all(np.abs(jpsreport_density - real_density_array) < acceptance_range):
        logging.info("density calculation with cut off option should be OK.")
    else:
        logging.critical(
            "density values with cut off option did not match result. Got {}. Expected {}".format(jpsreport_density,
                                                                                                  real_density_array))
        exit(FAILURE)


# ---------
# Test of IFD output when intersecting with geoemetry for Method D, I and J
# IDs of pedestrians that are in the measurement area for frame 109 are checked
# Corridor is choosen more narrow so that vornoi cells next to walls should intersect with the geometry
# Measurement area contains first and last row of these pedestrians
# Their individual density must be 1 person / (area of the rectangle)
# Note: Function is only applicable for one specified scenario since the distance to the gemoetry needs to be known
# ---------
def test_IFD_geometry_intersection(method, trajfile):
    jpsreport_result_file = os.path.join('./Output',
                                         'Fundamental_Diagram',
                                         'IndividualFD',
                                         'IFD_%s_%s_id_1.dat' % (method, trajfile)
                                         )

    if not os.path.exists(jpsreport_result_file):
        logging.critical("jpsreport did not output results correctly.")
        exit(FAILURE)

    # set test configuration
    # accepted error
    acceptance_range = 0.001
    # frame when inner voronoi cells of 16 people are entirely in the measurement area
    frame = 109

    # density can be calculated based on the geometry and the corresponding rectangle
    real_density_first_row = 1 / (1.25 * 1.5)
    real_density_last_row = 1 / 1.5

    jpsreport_data = np.loadtxt(jpsreport_result_file, usecols=(0, 1, 2, 3, 4, 5))

    # get data for specified frame
    jpsreport_frame_data = jpsreport_data[jpsreport_data[:, 0] == frame][:, :]

    # get density data for first and last row
    jpsreport_density_first_row = jpsreport_frame_data[jpsreport_frame_data[:, 1] < 6][:,
                                  5]  # ped IDs of first row in measurement area: 2,3,4,5
    jpsreport_density_last_row = jpsreport_frame_data[jpsreport_frame_data[:, 1] > 31][:,
                                 5]  # ped IDs of last row in measurement area: 32,33,34,35

    real_density_array_first_row = np.ones(np.size(jpsreport_density_first_row)) * real_density_first_row
    real_density_array_last_row = np.ones(np.size(jpsreport_density_first_row)) * real_density_last_row

    if np.all(np.abs(jpsreport_density_first_row - real_density_array_first_row) < acceptance_range):
        logging.info("density calculation when intersecting geometry should be OK.")
    else:
        logging.critical(
            "density values for pedestrians in first row did not match result. Got {}. Expected {}".format(
                jpsreport_density_first_row, real_density_array_first_row))
        exit(FAILURE)

    if np.all(np.abs(jpsreport_density_last_row - real_density_array_last_row) < acceptance_range):
        logging.info("density calculation when intersecting geometry should be OK.")
    else:
        logging.critical(
            "density values for pedestrians in last row did not match result. Got {}. Expected {}".format(
                jpsreport_density_last_row, real_density_array_last_row))
        exit(FAILURE)
