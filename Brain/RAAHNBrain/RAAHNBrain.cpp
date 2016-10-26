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

RAAHNBrain::RAAHNBrain(int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT) :
	AbstractBrain(_nrInNodes, _nrOutNodes, _nrHiddenNodes, _PT) {

	NeuralNetwork ann;

	// Initialize groups.
	int input_idx = ann.AddNeuronGroup(_nrInNodes, NeuronGroup::Type::INPUT);
	int hidden_idx = ann.AddNeuronGroup(10, NeuronGroup::Type::HIDDEN); // TODO: Temp...
	int output_idx = ann.AddNeuronGroup(_nrOutNodes, NeuronGroup::Type::OUTPUT);

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

void RAAHNBrain::update() 
{


	ann.PropagateSignal();
	ann.Train();

	nodes = nextNodes;
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
	return shared_ptr<AbstractBrain>();
}

void RAAHNBrain::initalizeGenome(shared_ptr<AbstractGenome> _genome)
{
}
