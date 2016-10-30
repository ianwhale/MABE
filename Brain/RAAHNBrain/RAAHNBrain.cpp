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
	// In order to keep with MABE convention, we'll treat hidden nodes as inputs and outputs. (TODO: Perhaps this is overkill since we have history buffer...)
	int input_idx = ann.AddNeuronGroup(_nrInNodes + _nrHiddenNodes, NeuronGroup::Type::INPUT);
	int hidden_idx = ann.AddNeuronGroup(10, NeuronGroup::Type::HIDDEN); // TODO: Temp...
	output_idx = ann.AddNeuronGroup(_nrOutNodes + _nrHiddenNodes, NeuronGroup::Type::OUTPUT);

	NeuronGroup::Identifier input;
	input.index = input_idx;
	input.type = NeuronGroup::Type::INPUT;

	NeuronGroup::Identifier hidden;
	hidden.index = hidden_idx;
	hidden.type = NeuronGroup::Type::HIDDEN;

	NeuronGroup::Identifier output;
	output.index = output_idx;
	output.type = NeuronGroup::Type::OUTPUT;

	// Use Hebbian training method. 
	ConnectionGroup::TrainFunctionType trainMethod = TrainingMethod::HebbianTrain;

	unsigned modSig = ModulationSignal::AddSignal();

	// Connect the groups.
	ann.ConnectGroups(&input, &hidden, trainMethod, (int)modSig, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);
	ann.ConnectGroups(&hidden, &output, trainMethod, (int)modSig, DEFAULT_SAMPLE_COUNT, DEFAULT_MODULATION_INDEX, true);

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
	vector<double> inputs_and_hidden;

	for (int i = 0; i < nrInNodes; i++) // Get inputs.
	{ 
		inputs_and_hidden.push_back(nodes[i]);
	}
	for (unsigned i = nrInNodes + nrOutNodes - 1; i < nodes.size(); i++) 
	{
		inputs_and_hidden.push_back(nodes[i]);
	}

	//cout << "Inputs:"; printNodes(nodes);

	ann.AddExperience(inputs_and_hidden);

	ann.PropagateSignal();

	double out;
	int neuron_iter = 0;

	// Set the results. 
	for (unsigned i = nrInNodes - 1; i < nodes.size(); i++)
	{
		out = ann.GetOutputValue(output_idx, neuron_iter);
		nodes[i] = out;
		neuron_iter++;
	}

	//cout << "Outputs: "; printNodes(nodes);
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
	return make_shared<RAAHNBrain>(RAAHNBrain(nrInNodes, nrOutNodes, nrHiddenNodes, PT));
}

void RAAHNBrain::initalizeGenome(shared_ptr<AbstractGenome> _genome)
{
	// TODO: Temp
}
