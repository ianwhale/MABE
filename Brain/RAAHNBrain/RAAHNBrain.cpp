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

using namespace std;

RAAHNBrain::RAAHNBrain(int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT) :
	AbstractBrain(_nrInNodes, _nrOutNodes, _nrHiddenNodes, _PT) {
	// Initialize groups.
	ann = make_unique<NeuralNetwork>(DEFAULT_HISTORY_BUFFER_SIZE, DEFAULT_OUTPUT_NOISE_MAGNITUDE, DEFAULT_WEIGHT_NOISE_MAGNITUDE, DEFAULT_NOVELTY_USE);

	int input_idx = ann->AddNeuronGroup(_nrInNodes - 2, NeuronGroup::Type::INPUT);

	int hiddenLayer = 5;

	if (_nrInNodes <= 5) {
		hiddenLayer = _nrInNodes - 1;
	}

	int hidden_idx = ann->AddNeuronGroup(hiddenLayer, NeuronGroup::Type::HIDDEN);
	output_idx = ann->AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);

	NeuronGroup::Identifier input;
	input.index = input_idx;
	input.type = NeuronGroup::Type::INPUT;

	NeuronGroup::Identifier hidden;
	hidden.index = hidden_idx;
	hidden.type = NeuronGroup::Type::HIDDEN;

	NeuronGroup::Identifier output;
	output.index = output_idx;
	output.type = NeuronGroup::Type::OUTPUT;

	ConnectionGroup::TrainFunctionType hebbTrain = TrainingMethod::HebbianTrain;
	ConnectionGroup::TrainFunctionType autoTrain = TrainingMethod::AutoencoderTrain;

	unsigned modIndex = ModulationSignal::AddSignal();

	// Connect the groups.
	ann->ConnectGroups(&input, &hidden, autoTrain, (int)modIndex, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);
	ann->ConnectGroups(&hidden, &output, hebbTrain, (int)modIndex, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);

	// Add noise.
	ann->SetOutputNoiseMagnitude(DEFAULT_OUTPUT_NOISE_MAGNITUDE);
	ann->SetWeightNoiseMagnitude(DEFAULT_WEIGHT_NOISE_MAGNITUDE);
}

RAAHNBrain::RAAHNBrain(shared_ptr<AbstractGenome> genome, int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT) :
	AbstractBrain(_nrInNodes, _nrOutNodes, _nrHiddenNodes, _PT) {
	// Initialize groups.
	ann = make_unique<NeuralNetwork>(DEFAULT_HISTORY_BUFFER_SIZE, DEFAULT_OUTPUT_NOISE_MAGNITUDE, DEFAULT_WEIGHT_NOISE_MAGNITUDE, DEFAULT_NOVELTY_USE);

	auto genomeHandler = genome->newHandler(genome);

	double trainingRate = genomeHandler->readDouble(0, 0.2);

	int hiddenLayer = genomeHandler->readInt(1, 7);

	//cout << "Number of nodes in hidden layer: " << hiddenLayer << endl;

	vector<double> hiddenWeights;
	//cout << "Weights: ";
	// get a weight for every connection in the autoencoder
	for (unsigned i = 0; i < hiddenLayer*7; i++) {
		hiddenWeights.push_back(genomeHandler->readDouble(0, 1));
		//cout << hiddenWeights.back() << " ";
	}
	//cout << endl;

	int input_idx = ann->AddNeuronGroup(_nrInNodes, NeuronGroup::Type::INPUT);

	int hidden_idx = ann->AddNeuronGroup(hiddenLayer, NeuronGroup::Type::HIDDEN);
	output_idx = ann->AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);

	NeuronGroup::Identifier input;
	input.index = input_idx;
	input.type = NeuronGroup::Type::INPUT;

	NeuronGroup::Identifier hidden;
	hidden.index = hidden_idx;
	hidden.type = NeuronGroup::Type::HIDDEN;

	NeuronGroup::Identifier output;
	output.index = output_idx;
	output.type = NeuronGroup::Type::OUTPUT;

	ConnectionGroup::TrainFunctionType hebbTrain = TrainingMethod::HebbianTrain;
	ConnectionGroup::TrainFunctionType autoTrain = TrainingMethod::AutoencoderTrain;

	unsigned modSig = ModulationSignal::AddSignal();

	// Connect the groups.
	ann->ConnectGroups(&input, &hidden, autoTrain, (int)modSig, DEFAULT_SAMPLE_COUNT, trainingRate, true, hiddenWeights);
	ann->ConnectGroups(&hidden, &output, hebbTrain, (int)modSig, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);

	// Add noise.
	ann->SetOutputNoiseMagnitude(DEFAULT_OUTPUT_NOISE_MAGNITUDE);
	ann->SetWeightNoiseMagnitude(DEFAULT_WEIGHT_NOISE_MAGNITUDE);
}

void printNodes(vector<double> &print_me)
{
	for (auto thing : print_me) {
		cout << thing << " ";
	}
	cout << endl;
}

void RAAHNBrain::update()
{
	vector<double> inputs;


	for (int i = 2; i < nrInNodes; i++) // Get inputs.
	{ 
		inputs.push_back(nodes[inputNodesList[i]]);
	}

	ann->AddExperience(inputs);

	ann->PropagateSignal();

	//
	// Need to add modulation signal and then call ann.Train()
	//
	double signal = (nodes[inputNodesList[0]] + nodes[inputNodesList[1]]) / 2; // Average the modulation signals. 
	ModulationSignal::SetSignal(modIndex, signal);

	ann->Train();

	double out;
	// Set the results. 
	for (int i = 0; i < nrOutNodes; i++)
	{
		out = ann->GetOutputValue(output_idx, i);
		nodes[outputNodesList[i]] = round(out); // Binary output for some worlds might not be the desired output... 
	}
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
	auto genomeHandler = _genome->newHandler(_genome);
	for (int i = 0; i < 100; i++) {
		genomeHandler->writeInt(Random::getInt(0, 255), 0, 255);
	}
}
