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
	int input_idx = ann.AddNeuronGroup(_nrInNodes, NeuronGroup::Type::INPUT);

	int hiddenLayer = 5;
	if (_nrInNodes <= 5) {
		hiddenLayer = _nrInNodes - 1;
	}

	int hidden_idx = ann.AddNeuronGroup(hiddenLayer, NeuronGroup::Type::HIDDEN); 
	output_idx = ann.AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);

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
	ann.ConnectGroups(&input, &hidden, autoTrain, (int)modSig, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);
	ann.ConnectGroups(&hidden, &output, hebbTrain, (int)modSig, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);

	// Add noise.
	ann.SetOutputNoiseMagnitude(DEFAULT_OUTPUT_NOISE_MAGNITUDE);
	ann.SetWeightNoiseMagnitude(DEFAULT_WEIGHT_NOISE_MAGNITUDE);
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

	for (int i = 0; i < nrInNodes - 1; i++) // Get inputs.
	{ 
		inputs.push_back(nodes[inputNodesList[i]]);
	}

	ann.AddExperience(inputs);

	ann.PropagateSignal();

	double out;
	int neuron_iter = 0;

	// Set the results. 
	for (int i = 0; i < nrOutNodes; i++)
	{
		out = ann.GetOutputValue(output_idx, neuron_iter);
		nodes[outputNodesList[i]] = round(out); // Binary output for some worlds might not be the desired output... 
		neuron_iter++;
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
	// TODO: Temp
	return make_shared<RAAHNBrain>(nrInNodes, nrOutNodes, nrHiddenNodes, PT);
}

void RAAHNBrain::initalizeGenome(shared_ptr<AbstractGenome> _genome)
{
	// TODO: Temp
}
