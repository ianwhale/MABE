//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/ahnt/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/ahnt/MABE/wiki/License

#include "RAAHNBrain.h"
#include "./RAAHNLib/NeuronGroup.h"
#include <cmath>
#include <iostream>

shared_ptr<ParameterLink<double>> RAAHNBrain::outputNoisePL = Parameters::register_parameter("BRAIN_RAAHN-outputNoise", 0.1, "noise added to output values.");
shared_ptr<ParameterLink<double>> RAAHNBrain::weightNoisePL = Parameters::register_parameter("BRAIN_RAAHN-weightNoise", 0.1, "noise added to weight values.");
shared_ptr<ParameterLink<int>> RAAHNBrain::sampleCountPL = Parameters::register_parameter("BRAIN_RAAHN-sampleCount", 10, "number of samples used from novelty buffer when training the autoencoder (unsigned).");
shared_ptr<ParameterLink<int>> RAAHNBrain::historyBufferSizePL = Parameters::register_parameter("BRAIN_RAAHN-historyBufferSize", 25, "history buffer size (unsigned).");
shared_ptr<ParameterLink<int>> RAAHNBrain::hiddenNodesPL = Parameters::register_parameter("BRAIN_RAAHN-hiddenNodes", 7, "number of hidden nodes (unsigned).");
shared_ptr<ParameterLink<double>> RAAHNBrain::learningRatePL = Parameters::register_parameter("BRAIN_RAAHN-learningRate", 0.1, "learning rate for Hebbian layer.");
shared_ptr<ParameterLink<bool>> RAAHNBrain::evolvingPL = Parameters::register_parameter("BRAIN_RAAHN-evolving", true, "should we evolve the learners? If false, no genome manipulation will be done and only in-lifetime learning will occur.");

using namespace std;

RAAHNBrain::RAAHNBrain(int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT) :
	AbstractBrain(_nrInNodes, _nrOutNodes, _nrHiddenNodes, _PT) {

	evolving = (PT == nullptr) ? evolvingPL->lookup() : PT->lookupBool("BRAIN_RAAHN-evolving");
	outputNoise = (PT == nullptr) ? outputNoisePL->lookup() : PT->lookupDouble("BRAIN_RAAHN-outputNoise");
	weightNoise = (PT == nullptr) ? weightNoisePL->lookup() : PT->lookupDouble("BRAIN_RAAHN-weightNoise");
	sampleCount = (PT == nullptr) ? sampleCountPL->lookup() : PT->lookupInt("BRAIN_RAAHN-sampleCount");
	historyBufferSize = (PT == nullptr) ? historyBufferSizePL->lookup() : PT->lookupInt("BRAIN_RAAHN-historyBufferSize");
	hiddenNodes = (PT == nullptr) ? hiddenNodesPL->lookup() : PT->lookupInt("BRAIN_RAAHN-hiddenNodes");
	learningRate = (PT == nullptr) ? learningRatePL->lookup() : PT->lookupInt("BRAIN_RAAHN-learningRate");

	ann = make_unique<NeuralNetwork>(historyBufferSize, outputNoise, weightNoise, DEFAULT_NOVELTY_USE);

	int input_idx = ann->AddNeuronGroup(_nrInNodes, NeuronGroup::Type::INPUT);
	int hidden_idx = ann->AddNeuronGroup(hiddenNodes, NeuronGroup::Type::HIDDEN);
	//output_idx = ann->AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);
	leftOut_idx = ann->AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);
	rightOut_idx = ann->AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);

	NeuronGroup::Identifier input;
	input.index = input_idx;
	input.type = NeuronGroup::Type::INPUT;

	NeuronGroup::Identifier hidden;
	hidden.index = hidden_idx;
	hidden.type = NeuronGroup::Type::HIDDEN;

	//NeuronGroup::Identifier output;
	//output.index = output_idx;
	//output.type = NeuronGroup::Type::OUTPUT;

	NeuronGroup::Identifier leftOutput;
	leftOutput.index = output_idx;
	leftOutput.type = NeuronGroup::Type::OUTPUT;
	NeuronGroup::Identifier rightOutput;
	rightOutput.index = output_idx;
	rightOutput.type = NeuronGroup::Type::OUTPUT;

	ConnectionGroup::TrainFunctionType hebbTrain = TrainingMethod::HebbianTrain;
	ConnectionGroup::TrainFunctionType autoTrain = TrainingMethod::AutoencoderTrain;

	leftModIndex = ModulationSignal::AddSignal();
	rightModIndex = ModulationSignal::AddSignal();

	// Connect the groups.
	ann->ConnectGroups(&input, &hidden, autoTrain, (int)modIndex, sampleCount, learningRate, true);
	ann->ConnectGroups(&hidden, &leftOutput, hebbTrain, (int)leftModIndex, sampleCount, 0.1 /*Value not used in Hebbian layer.*/, true);
	ann->ConnectGroups(&hidden, &rightOutput, hebbTrain, (int)rightModIndex, sampleCount, 0.1 /*Value not used in Hebbian layer.*/, true);
	//ann->ConnectGroups(&input, &output, hebbTrain, (int)modIndex, sampleCount, 0.1, true);

	// Add noise.
	ann->SetOutputNoiseMagnitude(outputNoise);
	ann->SetWeightNoiseMagnitude(weightNoise);
}

RAAHNBrain::RAAHNBrain(shared_ptr<AbstractGenome> genome, int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT) :
	AbstractBrain(_nrInNodes, _nrOutNodes, _nrHiddenNodes, _PT) {
	evolving = (PT == nullptr) ? evolvingPL->lookup() : PT->lookupBool("BRAIN_RAAHN-evolving");
	outputNoise = (PT == nullptr) ? outputNoisePL->lookup() : PT->lookupDouble("BRAIN_RAAHN-outputNoise");
	weightNoise = (PT == nullptr) ? weightNoisePL->lookup() : PT->lookupDouble("BRAIN_RAAHN-weightNoise");
	sampleCount = (PT == nullptr) ? sampleCountPL->lookup() : PT->lookupInt("BRAIN_RAAHN-sampleCount");
	historyBufferSize = (PT == nullptr) ? historyBufferSizePL->lookup() : PT->lookupInt("BRAIN_RAAHN-historyBufferSize");
	hiddenNodes = (PT == nullptr) ? hiddenNodesPL->lookup() : PT->lookupInt("BRAIN_RAAHN-hiddenNodes"); 
	learningRate = (PT == nullptr) ? learningRatePL->lookup() : PT->lookupDouble("BRAIN_RAAHN-learningRate");


	vector<double> hiddenWeights;
	if (evolving)
	{
		auto genomeHandler = genome->newHandler(genome);
		learningRate =  genomeHandler->readDouble(0.1, 0.2);
		hiddenNodes =  genomeHandler->readInt(1, 8);

		// historyBufferSize = genomeHandler->readInt( ... );
		// sampleCount = genomeHandler->readInt( ... );

		// Anything else ... ?

		
		for (unsigned i = 0; i < hiddenNodes * 8; i++) {
			hiddenWeights.push_back(genomeHandler->readDouble(0, 1));
		}

	}

	ann = make_unique<NeuralNetwork>(historyBufferSize, outputNoise, weightNoise, DEFAULT_NOVELTY_USE);

	int input_idx = ann->AddNeuronGroup(_nrInNodes, NeuronGroup::Type::INPUT);
	int hidden_idx = ann->AddNeuronGroup(hiddenNodes, NeuronGroup::Type::HIDDEN);
	//output_idx = ann->AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);
	leftOut_idx = ann->AddNeuronGroup(1, NeuronGroup::Type::OUTPUT);
	rightOut_idx = ann->AddNeuronGroup(1, NeuronGroup::Type::OUTPUT);

	NeuronGroup::Identifier input;
	input.index = input_idx;
	input.type = NeuronGroup::Type::INPUT;

	NeuronGroup::Identifier hidden;
	hidden.index = hidden_idx;
	hidden.type = NeuronGroup::Type::HIDDEN;

	//NeuronGroup::Identifier output;
	//output.index = output_idx;
	//output.type = NeuronGroup::Type::OUTPUT;

	NeuronGroup::Identifier leftOutput;
	leftOutput.index = leftOut_idx;
	leftOutput.type = NeuronGroup::Type::OUTPUT;
	NeuronGroup::Identifier rightOutput;
	rightOutput.index = rightOut_idx;
	rightOutput.type = NeuronGroup::Type::OUTPUT;

	ConnectionGroup::TrainFunctionType hebbTrain = TrainingMethod::HebbianTrain;
	ConnectionGroup::TrainFunctionType autoTrain = TrainingMethod::AutoencoderTrain;

	leftModIndex = ModulationSignal::AddSignal();
	rightModIndex = ModulationSignal::AddSignal();

	// Connect the groups.
	ann->ConnectGroups(&input, &hidden, autoTrain, (int)modIndex, sampleCount, learningRate, true, hiddenWeights);
	ann->ConnectGroups(&hidden, &leftOutput, hebbTrain, (int)leftModIndex, sampleCount, learningRate /*Value not used in Hebbian layer.*/, true);
	ann->ConnectGroups(&hidden, &rightOutput, hebbTrain, (int)rightModIndex, sampleCount, learningRate /*Value not used in Hebbian layer.*/, true);
	//ann->ConnectGroups(&input, &output, hebbTrain, (int)modIndex, sampleCount, 0.1, true);

	// Add noise.
	ann->SetOutputNoiseMagnitude(outputNoise);
	ann->SetWeightNoiseMagnitude(weightNoise);
}

void RAAHNBrain::update()
{
	vector<double> inputs;
	for (int i = 3; i < nrInNodes; i++) // Get inputs.
	{ 
		inputs.push_back(nodes[inputNodesList[i]]);
	}

	ann->AddExperience(inputs);
	//cout << "1: " << ann->GetOutputValue(leftOut_idx, 0) << " " << ann->GetOutputValue(rightOut_idx, 0) << endl;
	ann->PropagateSignal();
	//cout << "2: " << ann->GetOutputValue(leftOut_idx, 0) << " " << ann->GetOutputValue(rightOut_idx, 0) << endl;
	//ann->PropagateSignal();
	//cout << "3: " << ann->GetOutputValue(leftOut_idx, 0) << " " << ann->GetOutputValue(rightOut_idx, 0) << endl;
	//ann->PropagateSignal();

	//double out;
	// Set the results. 
	//for (int i = 0; i < nrOutNodes; i++)
	//{
	//	out = round(ann->GetOutputValue(output_idx, i));  // Binary output for some worlds might not be the desired output... 
		//cout << ann->GetOutputValue(output_idx, i) << endl;
	//	nodes[outputNodesList[i]] = out;
	//}

	nodes[outputNodesList[0]] = ann->GetOutputValue(leftOut_idx, 0);
	nodes[outputNodesList[1]] = ann->GetOutputValue(rightOut_idx, 0);

	//cout << ann->GetOutputValue(leftOut_idx, 0) << " " << ann->GetOutputValue(rightOut_idx, 0) << endl;


	// cout << "<------------------------------------------------------------------->" << endl;

	// cout << "Weights before training: " << endl;
	//ann->printWeights();

	//double signal = (nodes[inputNodesList[0]] + nodes[inputNodesList[1]]) / 2; // Average the modulation signals. 
	//ModulationSignal::SetSignal(modIndex, signal);


	double leftSignal = (nodes[inputNodesList[0]] + nodes[inputNodesList[1]]) / 2;
	double rightSignal = (nodes[inputNodesList[0]] + nodes[inputNodesList[2]]) / 2;
	//double leftSignal = nodes[inputNodesList[1]];
	//double rightSignal = nodes[inputNodesList[2]];
	//cout << leftSignal << " " << rightSignal << endl;
	ModulationSignal::SetSignal(leftModIndex, leftSignal);
	ModulationSignal::SetSignal(rightModIndex, rightSignal);
	ann->Train();

	// cout << "Weights after training: " << endl;
	//ann->printWeights();
	//cout << endl;
}

string RAAHNBrain::description()
{
	return "RAAHN Brain\n";
}

DataMap RAAHNBrain::getStats()
{
	// Stats we may want:
	// Hidden layer stats: number of nodes, connections, their weights, etc...

	return DataMap();
}

shared_ptr<AbstractBrain> RAAHNBrain::makeBrainFromGenome(shared_ptr<AbstractGenome> _genome)
{
	return make_shared<RAAHNBrain>(_genome, nrInNodes, nrOutNodes, nrHiddenNodes, PT);
}

void RAAHNBrain::initalizeGenome(shared_ptr<AbstractGenome> _genome)
{
	if (!evolving)
	{
		return; // We don't want to initialize the genome if we're not evolving the learners. 
	}

	auto genomeHandler = _genome->newHandler(_genome);
	for (int i = 0; i < 100; i++) {
		genomeHandler->writeInt(Random::getInt(0, 255), 0, 255);
	}
}
