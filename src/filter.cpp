#include <iomanip>
#include <iostream>
#include <vector>

#include "AccumTime.h"
#include "Cfg.h"
#include "Cloth.h"
#include "filter.h"
#include "handlers.h"
#include "main_options.h"
#include "octree.h"
#include "SMRF.h"


void doSMRF(Cfg& config)
{
	if (config.isGoodRead())
	{
		std::istringstream cell_size(config.getValue("cell_size"));
		std::istringstream max_slope(config.getValue("max_slope"));
		std::istringstream max_window_size(config.getValue("max_window_size"));
		std::istringstream max_height(config.getValue("max_height"));

		cell_size >> SMRF::CELL_SIZE;
		max_slope >> SMRF::MAX_SLOPE;
		max_window_size >> SMRF::MAX_WINDOW_SIZE;
		max_height >> SMRF::MAX_HEIGHT;
	}

	std::vector<Lpoint> points = readPointCloud(mainOptions.inputFile);
	// Handle number of points
	handleNumberOfPoints(points);

	// Global Octree Creation
	float  maxRadius = 0.0;
	Vector center    = mbb(points, maxRadius);
	std::cout << "Building global octree...\n";
	Octree gOctree(center, maxRadius, points);

	double min[3], max[3];
	SMRF::getMinMaxCoordinates(points, min, max);
	SMRF::DTMGrid dtm = SMRF::smrf(points, gOctree, min, max);
	
	AccumTime::instance().report(std::cout);
}

void doCSF(Cfg& config)
{
	CSF csf{};

	if (config.isGoodRead())
	{
		std::string slop_smooth(config.getValue("slop_smooth"));
		
		bool ss = false;
		if (slop_smooth == "true" || slop_smooth == "True")
		{
			ss = true;
		}
		else if (slop_smooth == "false" || slop_smooth == "False")
		{
			ss = false;
		}
		else
		{
			if (atoi(slop_smooth.c_str()) == 0)
			{
				ss = false;
			}
			else
			{
				ss = true;
			}
		}
		std::string class_threshold(config.getValue("class_threshold"));
		std::string cloth_resolution(config.getValue("cloth_resolution"));
		std::string iterations(config.getValue("iterations"));
		std::string rigidness(config.getValue("rigidness"));
		std::string time_step(config.getValue("time_step"));
		
		//step 2 parameter setting
		csf.params.bSloopSmooth = ss;
		csf.params.class_threshold = atof(class_threshold.c_str());
		csf.params.cloth_resolution = atof(cloth_resolution.c_str());
		csf.params.interations = atoi(iterations.c_str());
		csf.params.rigidness = atoi(rigidness.c_str());
		csf.params.time_step = atof(time_step.c_str());
	}

	//step 1 Input the point cloud
	csf.readPointsFromFile(mainOptions.inputFile);

	//step3 do filtering
	std::vector<int> groundIndexes, offGroundIndexes;
	csf.m_inputFile = mainOptions.inputFile;
	csf.do_filtering(groundIndexes, offGroundIndexes, true);	
	
	AccumTime::instance().report(std::cout);

	int total = groundIndexes.size() + offGroundIndexes.size();
	std::cout << std::setprecision(2);
	std::cout << "Ground: " << groundIndexes.size()
			  << " ("
	          << static_cast<double>(groundIndexes.size()) / total * 100.0
			  << "%)\n"; 
	std::cout << "Non-ground: " << offGroundIndexes.size()
			  << " ("
	          << static_cast<double>(offGroundIndexes.size()) / total * 100.0
			  << "%)\n";

	csf.savePoints(groundIndexes, mainOptions.outputDirName + "/ground.txt");
	csf.savePoints(offGroundIndexes,  mainOptions.outputDirName + "/non-ground.txt");
}

void Filter::doFilter()
{
	Cfg cfg{};
	if (!mainOptions.configFile.empty())
	{
		cfg.readFile(mainOptions.configFile);
	}

	// create the directory
	createDirectory(mainOptions.outputDirName);

	// Print decimals
	std::cout << std::fixed;
	std::cout << std::setprecision(3);

	// execute the choosen algorithm
	switch (mainOptions.algorithm)
	{
	case Algorithm::CSF:
		doCSF(cfg);
		break;

	case Algorithm::SMRF:
		doSMRF(cfg);
		break;

	default:
		std::cout << "Empty or unrecognised algorithm\n";
		break;
	}
}