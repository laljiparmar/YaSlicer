#include "Renderer.h"
#include "ERM.h"
#include "Utils.h"

#include <PngFile.h>
#include <ErrorHandling.h>

#include <memory>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <algorithm>
#include <thread>
#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>

void WriteWhiteLayers(const Settings& settings)
{
	const auto outputDir = boost::filesystem::path(settings.outputDir);

	for (uint32_t i = 0; i < settings.whiteLayers; ++i)
	{
		auto filePath = (outputDir / GetOutputFileName(settings, i)).string();

		std::vector<uint8_t> data(settings.renderWidth * settings.renderHeight);
		const uint8_t WhiteColorPaletteIndex = 0xFF;
		std::fill(data.begin(), data.end(), WhiteColorPaletteIndex);
		WritePng(filePath, settings.renderWidth, settings.renderHeight, 8, data, CreateGrayscalePalette());
	}	
}

void RenderModel(Renderer& r, const Settings& settings)
{
	auto tStart = std::chrono::high_resolution_clock::now();
	const auto outputDir = boost::filesystem::path(settings.outputDir);

	WriteWhiteLayers(settings);
	
	uint32_t nSlice = 0;
	uint32_t imageNumber = settings.whiteLayers;
	r.FirstSlice();
	do
	{
		auto filePath = (outputDir / GetOutputFileName(settings, imageNumber++)).string();
		r.SavePng(filePath);

		if (settings.doOverhangAnalysis)
		{
			r.AnalyzeOverhangs(imageNumber-1);
		}

		if (settings.enableERM)
		{
			r.ERM();
			filePath = (outputDir / GetOutputFileName(settings, imageNumber++)).string();
			r.SavePng(filePath);
		}

		++nSlice;
	} while (r.NextSlice());

	WriteEnvisiontechConfig(settings, "job.cfg", nSlice);

	auto tRender = std::chrono::high_resolution_clock::now();
	auto renderTime = std::chrono::duration_cast<std::chrono::milliseconds>(tRender - tStart).count();
	std::cout << "Render: " << renderTime <<
		" ms, " << (nSlice * 1000.0) / renderTime << " FPS" << std::endl;
}

int main(int argc, char** argv)
{
	try
	{
		Settings settings;
		std::string configFile;

		namespace po = boost::program_options;
		// Declare a group of options that will be 
		// allowed only on command line
		po::options_description generic("generic options");
		generic.add_options()
			("help,h", "produce help message")
			("config,c", po::value<std::string>(&configFile)->default_value("config.cfg"), "slicing configuration file")
			;

		// Declare a group of options that will be 
		// allowed both on command line and in
		// config file
		po::options_description config("slicing configuration");
		config.add_options()
			("modelFile,m", po::value<std::string>(&settings.modelFile), "model to process")
			("outputDir,o", po::value<std::string>(&settings.outputDir), "output directory")

			("step", po::value<float>(&settings.step)->default_value(settings.step), "slicing step (mm)")

			("renderWidth", po::value<uint32_t>(&settings.renderWidth)->default_value(settings.renderWidth), "image x resolution")
			("renderHeight", po::value<uint32_t>(&settings.renderHeight)->default_value(settings.renderHeight), "image y resolution")
			("samples", po::value<uint32_t>(&settings.samples)->default_value(settings.samples), "samples per pixel")

			("plateWidth", po::value<float>(&settings.plateWidth)->default_value(settings.plateWidth), "platform width (mm)")
			("plateHeight", po::value<float>(&settings.plateHeight)->default_value(settings.plateHeight), "platform height (mm)")

			("doAxialDilate", po::value<bool>(&settings.doAxialDilate)->default_value(settings.doAxialDilate), "add pixels left and bottom on all contours")

			("doBinarize", po::value<bool>(&settings.doBinarize)->default_value(settings.doBinarize), "binarize final image")
			("binarizeThreshold", po::value<uint32_t>(&settings.binarizeThreshold)->default_value(settings.binarizeThreshold), "binarization threshold")

			("doOmniDirectionalDilate", po::value<bool>(&settings.doOmniDirectionalDilate)->default_value(settings.doOmniDirectionalDilate), "extend all contours by 1 pixel")
			("omniDilateSliceFactor", po::value<uint32_t>(&settings.omniDilateSliceFactor)->default_value(settings.omniDilateSliceFactor), "do omni directional dilate every N slice")
			("omniDilateScale", po::value<float>(&settings.omniDilateScale)->default_value(settings.omniDilateScale), "scale omni directional extended pixels color by some factor")

			("doSmallSpotsProcessing", po::value<bool>(&settings.doSmallSpotsProcessing)->default_value(settings.doSmallSpotsProcessing), "detect & process small spots")
			("smallSpotThreshold", po::value<float>(&settings.smallSpotThreshold)->default_value(settings.smallSpotThreshold), "maximum small spot area (mm^2)")
			//("dilateOnlySmallSpots", po::value<bool>(&settings.dilateOnlySmallSpots)->default_value(settings.dilateOnlySmallSpots), "dilate only small spots")
			("smallSpotColorScaleFactor", po::value<float>(&settings.smallSpotColorScaleFactor)->default_value(settings.smallSpotColorScaleFactor), "scale small spot color by some factor")

			("modelOffsetX", po::value<float>(&settings.modelOffset.x)->default_value(settings.modelOffset.x), "model X offset in pixels")
			("modelOffsetY", po::value<float>(&settings.modelOffset.y)->default_value(settings.modelOffset.y), "model Y offset in pixels")

			("doOverhangAnalysis,a", po::value<bool>(&settings.doOverhangAnalysis)->default_value(settings.doOverhangAnalysis), "analyze unsupported model parts")
			("maxSupportedDistance", po::value<float>(&settings.maxSupportedDistance)->default_value(settings.maxSupportedDistance), "maximum length of overhang upon previous layer (mm)")

			("enableERM,e", po::value<bool>(&settings.enableERM)->default_value(settings.enableERM), "enable ERM mode")
			("envisiontechTemplatesPath", po::value<std::string>(&settings.envisiontechTemplatesPath)->default_value(settings.envisiontechTemplatesPath), "envisiontech job templates path")

			("queue", po::value<uint32_t>(&settings.queue)->default_value(settings.queue), "PNG compression & write queue length (balance CPU-GPU load)")
			("whiteLayers", po::value<uint32_t>(&settings.whiteLayers)->default_value(settings.whiteLayers), "white layers count")

			("mirrorX", po::value<bool>(&settings.mirrorX)->default_value(settings.mirrorX), "mirror image horizontally")
			("mirrorY", po::value<bool>(&settings.mirrorY)->default_value(settings.mirrorY), "mirror image vertically")
			;

		po::options_description cmdline_options;
		cmdline_options.add(generic).add(config);

		po::options_description config_file_options;
		config_file_options.add(config);

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, cmdline_options), vm);
		po::notify(vm);

		if (vm.count("help") || argc < 2)
		{
			std::cout << "Yarilo slicer v0.83, 2016" << "\n";
			std::cout << cmdline_options << "\n";
			return 0;
		}

		po::store(po::parse_config_file<char>(configFile.c_str(), config_file_options), vm);
		po::notify(vm);

		if (settings.modelFile.empty())
		{
			std::cout << "No model to slice, exit" << "\n";
			return 0;
		}

		boost::filesystem::create_directories(settings.outputDir);

		Renderer r(settings);
		RenderModel(r, settings);
	}
	catch (const std::exception& e)
	{
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}