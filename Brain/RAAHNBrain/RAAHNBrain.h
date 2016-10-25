//  MABE is a product of The Hintze Lab @ MSU
//     for general research information:
//         hintzelab.msu.edu
//     for MABE documentation:
//         github.com/ahnt/MABE/wiki
//
//  Copyright (c) 2015 Michigan State University. All rights reserved.
//     to view the full license, visit:
//         github.com/ahnt/MABE/wiki/License

#pragma once
#include "../AbstractBrain.h"
#include "./RAAHNLib/NeuralNetwork.h"

class RAAHNBrain : public AbstractBrain
{
public:
	RAAHNBrain(int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT = nullptr);
	// RAAHNBrain(shared_ptr<AbstractGenome> genome, int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT = nullptr);

	virtual ~RAAHNBrain() = default;
	virtual void update() override;
	virtual string description() override;
	virtual DataMap getStats() override;
	virtual shared_ptr<AbstractBrain> makeBrainFromGenome(shared_ptr<AbstractGenome> _genome) override;
	virtual void initalizeGenome(shared_ptr<AbstractGenome> _genome) override;

	NeuralNetwork ann;
};

