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
#include <memory>

class RAAHNBrain : public AbstractBrain
{
public:
	static shared_ptr<ParameterLink<double>> outputNoisePL;
	static shared_ptr<ParameterLink<double>> weightNoisePL;
	static shared_ptr<ParameterLink<double>> learningRatePL;
	static shared_ptr<ParameterLink<int>> sampleCountPL;
	static shared_ptr<ParameterLink<int>> historyBufferSizePL;
	static shared_ptr<ParameterLink<int>> hiddenNodesPL;
	static shared_ptr<ParameterLink<bool>> evolvingPL;

	double outputNoise;
	double weightNoise;
	double learningRate;
	int hiddenNodes;
	int sampleCount;
	int historyBufferSize;
	bool evolving;

	const bool DEFAULT_NOVELTY_USE = true;
	
	RAAHNBrain(int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT = nullptr);
	RAAHNBrain(shared_ptr<AbstractGenome> genome, int _nrInNodes, int _nrOutNodes, int _nrHiddenNodes, shared_ptr<ParametersTable> _PT = nullptr);

	virtual ~RAAHNBrain() = default;
	virtual void update() override;
	virtual string description() override;
	virtual DataMap getStats() override;
	virtual shared_ptr<AbstractBrain> makeBrainFromGenome(shared_ptr<AbstractGenome> _genome) override;
	virtual void initalizeGenome(shared_ptr<AbstractGenome> _genome) override;

private:
	int output_idx;
	unsigned modIndex;
	unique_ptr<NeuralNetwork> ann;
};

inline shared_ptr<AbstractBrain> RAAHNBrain_brainFactory(int ins, int outs, int hidden, shared_ptr<ParametersTable> PT) {
	return make_shared<RAAHNBrain>(ins, outs, hidden, PT);
}

