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

	// We'll treat hidden nodes as inputs and output node types (TODO: for now...)
	ann.AddNeuronGroup(_nrInNodes + _nrHiddenNodes, NeuronGroup::Type::INPUT);
	ann.AddNeuronGroup(_nrOutNodes + _nrHiddenNodes, NeuronGroup::Type::OUTPUT);
}

void RAAHNBrain::update() 
{
	ann.PropagateSignal();
	ann.Train();
}

string RAAHNBrain::description()
{
	return string();
}

DataMap RAAHNBrain::getStats()
{
	return DataMap();
}

shared_ptr<AbstractBrain> RAAHNBrain::makeBrainFromGenome(shared_ptr<AbstractGenome> _genome)
{
	return shared_ptr<AbstractBrain>();
}

void RAAHNBrain::initalizeGenome(shared_ptr<AbstractGenome> _genome)
{
}
