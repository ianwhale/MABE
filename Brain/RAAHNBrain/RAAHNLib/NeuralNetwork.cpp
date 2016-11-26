#include "NeuralNetwork.h"
#include <vector>
#include <random>
#include <limits>
#include <algorithm>
#include <memory>

using std::vector;
using std::sort;
using std::shared_ptr;
using std::make_shared;

NeuralNetwork::NeuralNetwork()
{
	Construct(DEFAULT_HISTORY_BUFFER_SIZE, DEFAULT_NOISE_MAGNITUDE, DEFAULT_NOISE_MAGNITUDE, DEFAULT_NOVELTY_USE);
}

NeuralNetwork::NeuralNetwork(unsigned historySize, bool useNoveltyBuffer)
{
	if (historySize > 0)
		Construct(historySize, DEFAULT_NOISE_MAGNITUDE, DEFAULT_NOISE_MAGNITUDE, useNoveltyBuffer);
	else
		Construct(DEFAULT_HISTORY_BUFFER_SIZE, DEFAULT_NOISE_MAGNITUDE, DEFAULT_NOISE_MAGNITUDE, useNoveltyBuffer);
}

NeuralNetwork::NeuralNetwork(double outputNoiseMag, double weightNoiseMag)
{
	Construct(DEFAULT_HISTORY_BUFFER_SIZE, outputNoiseMag, weightNoiseMag, DEFAULT_NOVELTY_USE);
}

NeuralNetwork::NeuralNetwork(unsigned historySize, double outputNoiseMag, double weightNoiseMag, bool useNoveltyBuffer)
{
	if (historySize > 0)
		Construct(historySize, outputNoiseMag, weightNoiseMag, useNoveltyBuffer);
	else
		Construct(DEFAULT_HISTORY_BUFFER_SIZE, outputNoiseMag, weightNoiseMag, useNoveltyBuffer);
}

//Adds a training sample to the history buffer.
void NeuralNetwork::AddExperience(vector<double> newExperience)
{
	//Check which buffer to use.
	if (useNovelty)
	{
		//Allocate a NoveltyBufferOccupant for the new experience.
		shared_ptr<NoveltyBufferOccupant> newOccupant = make_shared<NoveltyBufferOccupant>();
		newOccupant->experience = newExperience;
		vector<shared_ptr<DistanceDescription>> newDistances = ComputeNewDistances(newOccupant);

		//Check if the buffer is at capacity.
		if (noveltyBuffer.size() == historyBufferSize)
		{
			shared_ptr<NoveltyBufferOccupant> leastNovel = noveltyBuffer[0];

			//If the new experience is more novel than the least novel
			//then remove the least novel than add the new experience.
			if (newOccupant->noveltyScore > leastNovel->noveltyScore)
			{
				RemoveNoveltyOccupant(leastNovel);
				//Add the new experience taking into account its newly introduced distances.
				AddNoveltyOccupant(newOccupant, newDistances);
			}
		}
		//Buffer not at capacity, just add the new experience.
		else
			AddNoveltyOccupant(newOccupant, newDistances);
	}
	else
	{
		//If the buffer is full, pop the back to make room for the new experience.
		if (historyBuffer.size() == historyBufferSize)
			historyBuffer.pop_back();

		//Add the new experience.
		historyBuffer.push_front(newExperience);
	}

	SetExperience(newExperience);
}

//Public access for setting an experience.
void NeuralNetwork::SetExperience(unsigned index)
{
	//Check which buffer to use.
	if (useNovelty)
	{
		//Make sure the index is in the bounds of the novelty buffer.
		if (index >= noveltyBuffer.size())
			return;

		SetExperience(noveltyBuffer[(int)index]->experience);
	}
	else
	{
		//Make sure the index is in the bounds of the history buffer.
		if (index >= historyBuffer.size())
			return;

		SetExperience(historyBuffer[((int)index)]);
	}
}

//Propagates the inputs completely and gets output.
void NeuralNetwork::PropagateSignal()
{
	//Reset the computed state of hidden layer neurons.
	//Also reset the values for hidden and output neurons.
	for (unsigned i = 0; i < hiddenGroups.size(); i++)
	{
		hiddenGroups[i]->computed = false;
		hiddenGroups[i]->Reset();
	}

	for (unsigned i = 0; i < outputGroups.size(); i++)
		outputGroups[i]->Reset();

	for (unsigned i = 0; i < outputGroups.size(); i++)
		outputGroups[i]->ComputeSignal();
}

//Returns autoencoder error.
void NeuralNetwork::Train()
{
	double error = 0.0;

	for (unsigned i = 0; i < inputGroups.size(); i++)
		error += inputGroups[i]->TrainRecent();

	for (unsigned i = 0; i < hiddenGroups.size(); i++)
		error += hiddenGroups[i]->TrainRecent();

	for (unsigned i = 0; i < inputGroups.size(); i++)
		error += inputGroups[i]->TrainSeveral();

	for (unsigned i = 0; i < hiddenGroups.size(); i++)
		error += hiddenGroups[i]->TrainSeveral();

	UpdateOnlineError(error);
}

//Resets the weights and every neuron of the neural network.
void NeuralNetwork::Reset()
{
	for (unsigned x = 0; x < allListGroups.size(); x++)
	{
		for (unsigned y = 0; y < allListGroups[x]->size(); y++)
		{
			(*allListGroups[x])[y]->Reset();
			(*allListGroups[x])[y]->ResetOutgoingGroups();
		}
	}

	if (useNovelty)
		noveltyBuffer.clear();
	else
		historyBuffer.clear();

	errorBuffer.clear();

	averageError = 0.0;
}

//Sets the maximum weight value. Ignores sign.
void NeuralNetwork::SetWeightCap(double cap)
{
	weightCap = abs(cap);
}

void NeuralNetwork::SetOutputNoiseMagnitude(double outputNoiseMag)
{
	outputNoiseMagnitude = outputNoiseMag;
	outputNoiseRange = outputNoiseMag * DOUBLE_MAGNITUDE;
}

void NeuralNetwork::SetWeightNoiseMagnitude(double weightNoiseMag)
{
	weightNoiseMagnitude = weightNoiseMag;
	weightNoiseRange = weightNoiseMag * DOUBLE_MAGNITUDE;
}

//Returns whether the output was able to be set.
bool NeuralNetwork::SetOutput(unsigned groupIndex, unsigned index, double value)
{
	if (groupIndex >= outputGroups.size())
		return false;

	int groupIndexi = (int)groupIndex;

	if (index >= outputGroups[groupIndexi]->neurons.size())
		return false;

	outputGroups[groupIndexi]->neurons[(int)index] = value;

	return true;
}

//Returns false if one or both of the groups do not exist.
//Returns true if the groups could be connected.
//Sample count refers to how many training samples should be used each time Train() is called.
bool NeuralNetwork::ConnectGroups(NeuronGroup::Identifier *input, NeuronGroup::Identifier *output,
	ConnectionGroup::TrainFunctionType trainMethod, int modulationIndex,
	unsigned sampleCount, double learningRate, bool useBias, vector<double> &presetWeights)
{
	if (!VerifyIdentifier(input) || !VerifyIdentifier(output))
		return false;

	shared_ptr<NeuronGroup> iGroup = (*allListGroups[(int)input->type])[(int)input->index];
	shared_ptr<NeuronGroup> oGroup = (*allListGroups[(int)output->type])[(int)output->index];

	shared_ptr<ConnectionGroup> cGroup = make_shared<ConnectionGroup>(this, iGroup, oGroup, useBias);
	cGroup->sampleUsageCount = sampleCount;
	cGroup->SetLearningRate(learningRate);
	cGroup->SetTrainingMethod(trainMethod);
	cGroup->SetModulationIndex(modulationIndex);

	double neuronInOutCount = (double)(iGroup->neurons.size() + oGroup->neurons.size());

	if (useBias)
		neuronInOutCount++;

	int oCount = oGroup->neurons.size();
	for (unsigned x = 0; x < iGroup->neurons.size(); x++)
	{
		for (unsigned y = 0; y < oGroup->neurons.size(); y++)
		{
			double range = sqrt(WEIGHT_RANGE_SCALE / neuronInOutCount);
			//Keep in the range of [-range, range]

			double weight;
			if (presetWeights.size() > x*oCount + y) {
				weight = (presetWeights[x*oCount + y] * range * DOUBLE_WEIGHT_RANGE) - range;
			}
			else {
				weight = (NextDouble() * range * DOUBLE_WEIGHT_RANGE) - range;
			}
			cGroup->AddConnection(x, y, weight);
		}
	}

	if (useBias)
		cGroup->AddBiasWeights((unsigned)oGroup->neurons.size());

	//sampleCount of zero refers to training off of the most recent experience.
	iGroup->AddOutgoingGroup(cGroup, sampleCount == 0);
	oGroup->AddIncomingGroup(cGroup);

	return true;
}

//Gets the number of neurons in a group. Returns 0 if the group is invalid.
unsigned NeuralNetwork::GetGroupNeuronCount(NeuronGroup::Identifier *ident)
{
	if (!VerifyIdentifier(ident))
		return 0;

	return (*allListGroups[(int)ident->type])[ident->index]->GetNeuronCount();
}

//Returns the index of the neuron group.
int NeuralNetwork::AddNeuronGroup(unsigned neuronCount, NeuronGroup::Type type)
{
	if (!VerifyType(type))
		return NeuronGroup::INVALID_NEURON_INDEX;

	shared_ptr<NeuronGroup> newGroup = make_shared<NeuronGroup>(this, type);
	newGroup->AddNeurons(neuronCount);
	newGroup->type = type;

	switch (type)
	{
	case NeuronGroup::Type::INPUT:
	{
		inputGroups.push_back(newGroup);
		int groupIndex = inputGroups.size() - 1;

		newGroup->index = groupIndex;

		return groupIndex;
	}
	case NeuronGroup::Type::HIDDEN:
	{
		hiddenGroups.push_back(newGroup);
		int groupIndex = hiddenGroups.size() - 1;

		newGroup->index = groupIndex;

		return groupIndex;
	}
	case NeuronGroup::Type::OUTPUT:
	{
		outputGroups.push_back(newGroup);
		int groupIndex = outputGroups.size() - 1;

		newGroup->index = groupIndex;

		return groupIndex;
	}
	default:
	{
		return NeuronGroup::INVALID_NEURON_INDEX;
	}
	}
}

double NeuralNetwork::GetWeightCap()
{
	return weightCap;
}

//Returns double.Nan if the neuron or neuron group does not exist.
double NeuralNetwork::GetNeuronValue(NeuronGroup::Identifier *ident, unsigned neuronIndex)
{
	if (!VerifyIdentifier(ident))
		return std::numeric_limits<double>::quiet_NaN();

	int typei = (int)ident->type;
	int indexi = (int)ident->index;

	if (neuronIndex >= (*allListGroups[typei])[indexi]->neurons.size())
		return std::numeric_limits<double>::quiet_NaN();

	return (*allListGroups[typei])[indexi]->neurons[(int)neuronIndex];
}

//Returns double.Nan if the neuron or neuron group does not exist.
double NeuralNetwork::GetOutputValue(unsigned groupIndex, unsigned index)
{
	if (groupIndex >= outputGroups.size())
		return std::numeric_limits<double>::quiet_NaN();

	if (index >= outputGroups[(int)groupIndex]->neurons.size())
		return std::numeric_limits<double>::quiet_NaN();

	return outputGroups[(int)groupIndex]->neurons[(int)index];
}

//Returns the sum of the squared reconstruction error for the current sample.
double NeuralNetwork::CalculateBatchError()
{
	PropagateSignal();

	double error = 0.0;

	for (unsigned i = 0; i < inputGroups.size(); i++)
		error += inputGroups[i]->GetReconstructionError();

	for (unsigned i = 0; i < hiddenGroups.size(); i++)
		error += hiddenGroups[i]->GetReconstructionError();

	return error;
}

//Returns the average reconstruction error for the entire history buffer.
double NeuralNetwork::GetBatchError()
{
	double error = 0.0;

	for (vector<double> sample : historyBuffer)
	{
		SetExperience(sample);

		error += CalculateBatchError();
	}

	error /= historyBuffer.size();

	return error;
}

//Returns the average reconstruction error over the past [historyBufferSize] ticks.
double NeuralNetwork::GetOnlineError()
{
	return averageError;
}

//Get neuron values of a neuron group.
vector<double> NeuralNetwork::GetNeuronValues(NeuronGroup::Identifier *nGroup)
{
	if (!VerifyIdentifier(nGroup))
		return vector<double>();

	vector<double> neuronValues = vector<double>();

	for (unsigned i = 0; i < (*allListGroups[(int)nGroup->type])[nGroup->index]->neurons.size(); i++)
		neuronValues.push_back((*allListGroups[(int)nGroup->type])[nGroup->index]->neurons[i]);

	return neuronValues;
}

//Get the strength of connections in a connection group.
vector<double> NeuralNetwork::GetWeights(NeuronGroup::Identifier *fromGroup, NeuronGroup::Identifier *toGroup)
{
	if (!VerifyIdentifier(fromGroup) || !VerifyIdentifier(toGroup))
		return vector<double>();

	return (*allListGroups[(int)fromGroup->type])[fromGroup->index]->GetWeights(toGroup);
}

//Returns the Ids of all groups connected by outgoing connections to the specifed group.
vector<NeuronGroup::Identifier> NeuralNetwork::GetGroupsConnected(NeuronGroup::Identifier *connectedTo)
{
	if (!VerifyIdentifier(connectedTo))
		return vector<NeuronGroup::Identifier>();

	return (*allListGroups[(int)connectedTo->type])[connectedTo->index]->GetGroupsConnected();
}

//Constructs NeuralNetwork.
void NeuralNetwork::Construct(unsigned historySize, double outputNoiseMag, double weightNoiseMag, bool useNoveltyBuffer)
{
	useNovelty = useNoveltyBuffer;

	historyBufferSize = historySize;

	outputNoiseMagnitude = outputNoiseMag;
	weightNoiseMagnitude = weightNoiseMag;
	outputNoiseRange = outputNoiseMagnitude * DOUBLE_MAGNITUDE;
	weightNoiseRange = weightNoiseMagnitude * DOUBLE_MAGNITUDE;

	weightCap = std::numeric_limits<double>::max();
	averageError = 0.0;

	int historyBufferSizei = (int)historyBufferSize;

	if (useNovelty)
		noveltyBuffer = vector<shared_ptr<NoveltyBufferOccupant>>();
	else
		historyBuffer = deque<vector<double>>();

	errorBuffer = deque<double>();

	allListGroups = vector<shared_ptr<vector<shared_ptr<NeuronGroup>>>>();

	inputGroups = vector<shared_ptr<NeuronGroup>>();
	hiddenGroups = vector<shared_ptr<NeuronGroup>>();
	outputGroups = vector<shared_ptr<NeuronGroup>>();

	allListGroups.push_back(make_shared<vector<shared_ptr<NeuronGroup>>>(inputGroups));
	allListGroups.push_back(make_shared<vector<shared_ptr<NeuronGroup>>>(hiddenGroups));
	allListGroups.push_back(make_shared<vector<shared_ptr<NeuronGroup>>>(outputGroups));
}

//Set the inputs of the neural network to a given experience.
void NeuralNetwork::SetExperience(vector<double> sample)
{
	int dataCount = sample.size();
	int sampleIndex = 0;

	for (unsigned x = 0; x < inputGroups.size(); x++)
	{
		if (dataCount <= 0)
			break;

		int neuronCount = inputGroups[x]->neurons.size();

		//If there is to little data use only what is provided.
		if (dataCount < neuronCount)
			neuronCount = dataCount;

		for (int y = 0; y < neuronCount; y++)
		{
			inputGroups[x]->neurons[y] = sample[sampleIndex];
			sampleIndex++;
		}

		dataCount -= neuronCount;
	}
}

void NeuralNetwork::UpdateOnlineError(double currentError)
{
	//Make sure there is at least one error value.
	if (errorBuffer.size() < 1)
	{
		//Just add the new error, nothing else.
		errorBuffer.push_front(currentError);
		return;
	}

	//Convert the error count to a double.
	double errorCount = (double)(errorBuffer.size());

	//If the error queue is not filled to capacity,
	//the average has to be calculated by summing up all the errors.
	if (errorBuffer.size() < historyBufferSize)
	{
		double errorSum = 0.0;

		for (double error : errorBuffer)
			errorSum += error;

		averageError = errorSum / errorCount;
	}
	//errorCount does not change. Error can be calculated by removing
	//the first error and adding the new error, each over errorCount.
	else
	{
		averageError -= errorBuffer.back() / errorCount;
		averageError += currentError / errorCount;

		//Get rid of the old error value.
		errorBuffer.pop_back();
	}

	//Add the new error.
	errorBuffer.push_front(currentError);
}

//Add a new experience to the novelty buffer.
void NeuralNetwork::AddNoveltyOccupant(shared_ptr<NoveltyBufferOccupant> newOccupant, const vector<shared_ptr<DistanceDescription>>& distDescriptions)
{
	//Add the distance for the new occupant to each other and to itself if needed.
	for (unsigned i = 0; i < noveltyBuffer.size(); i++)
	{
		shared_ptr<DistanceDescription> newDes = make_shared<DistanceDescription>();
		newDes->distance = distDescriptions[i]->distance;
		newDes->distanceOwner = newOccupant;

		TryInsertDistance(noveltyBuffer[i], newDes);
	}

	noveltyBuffer.push_back(newOccupant);

	UpdateNoveltyScores();
	sort(noveltyBuffer.begin(), noveltyBuffer.end());
}

//Insert the distance if it is closer than the current farthest distance.
#pragma optimize( "", off)
void NeuralNetwork::TryInsertDistance(shared_ptr<NoveltyBufferOccupant> occupant, shared_ptr<DistanceDescription> distDesc)
{
	//Only check if a distance cache removal is needed if the distance cache is full.
	if (occupant->distanceDescriptions.size() >= N_NEAREST)
	{
		//Don't insert the distance if it is farther than the current farthest distance.
		if (distDesc->distance > occupant->distanceDescriptions.back()->distance)
			return;

		//Check if the distance cache is full. Make room for the new distance.
		if (occupant->distanceDescriptions.size() == N_KEEP)
			occupant->distanceDescriptions.pop_back();
	}

	//Insert the distance into the sorted distance list of the occupant->
	// initially used r-iterator, but I want make sure that I could insert AFTER an element
	/*
	for (auto it = occupant->distanceDescriptions.begin(); it != occupant->distanceDescriptions.end(); it++)
	{
	//Check where the new distance should be placed.
	if (distDesc->distance > (*it)->distance)
	{
	occupant->distanceDescriptions.insert(it+1, distDesc);
	return;
	}
	}
	//The new distance is the new nearest.
	occupant->distanceDescriptions.insert(occupant->distanceDescriptions.begin(), distDesc);
	*/

	// just sort the list so that this function doesn't get optimized out again
	occupant->distanceDescriptions.push_back(distDesc);
	sort(occupant->distanceDescriptions.begin(), occupant->distanceDescriptions.end());
}

//Remove an experience from the novelty buffer.
void NeuralNetwork::RemoveNoveltyOccupant(shared_ptr<NoveltyBufferOccupant> oldOccupant)
{
	auto itr = find(noveltyBuffer.begin(), noveltyBuffer.end(), oldOccupant);
	if (itr != noveltyBuffer.end())
		noveltyBuffer.erase(itr); // TODO: verify

	for (auto occupant : noveltyBuffer)
	{
		for (auto it = occupant->distanceDescriptions.begin(); it != occupant->distanceDescriptions.end(); it++)
		{
			//Remove the distance if it is the distance to the removed occupant->
			if ((*it)->distanceOwner == oldOccupant)
			{
				occupant->distanceDescriptions.erase(it);
				break;
			}
		}

		//Recompute novelty scores for the occupant if it the novelty buffer 
		//has experiences that can fill the rest of the occupant's nearest distances.
		if (noveltyBuffer.size() > N_KEEP && occupant->distanceDescriptions.size() < N_NEAREST)
			ComputeDistances(occupant);
	}
}

void NeuralNetwork::UpdateNoveltyScores()
{
	for (auto occupant : noveltyBuffer)
	{
		unsigned distancesToSum = occupant->distanceDescriptions.size();

		if (distancesToSum > N_NEAREST)
			distancesToSum = (int)N_NEAREST;

		occupant->noveltyScore = 0.0;

		//LinkedListNode<DistanceDescription> distanceIterator = occupant->distanceDescriptions.First;
		auto itr = occupant->distanceDescriptions.begin();

		for (unsigned i = 0; i < distancesToSum; i++)
		{
			occupant->noveltyScore += (*itr)->distance;
			itr++;
		}
	}
}

//Expensive computation of distances for an experience already in buffer.
void NeuralNetwork::ComputeDistances(shared_ptr<NoveltyBufferOccupant> occupant)
{
	if (noveltyBuffer.size() == 0)
		return;

	//Only need one less than the number of novelty buffer occupants.
	vector<shared_ptr<DistanceDescription>> distanceDescriptions = vector<shared_ptr<DistanceDescription>>();

	for (shared_ptr<NoveltyBufferOccupant> currentOccupant : noveltyBuffer)
	{
		//Don't include the least novel occupant->
		if (currentOccupant != occupant)
		{
			double distance = ExpDistance(occupant->experience, currentOccupant->experience);

			shared_ptr<DistanceDescription> newDescription = make_shared<DistanceDescription>();
			newDescription->distance = distance;
			newDescription->distanceOwner = currentOccupant;

			distanceDescriptions.push_back(newDescription);
		}
	}

	//distanceDescriptions.Sort();
	sort(distanceDescriptions.begin(), distanceDescriptions.end());

	//Reset distances.
	occupant->distanceDescriptions.clear();

	//This method is only called when noveltyBuffer.size() > N_KEEP.
	for (unsigned i = 0; i < N_KEEP; i++)
		occupant->distanceDescriptions.push_back(distanceDescriptions[i]);
}

//Makes sure a type is INPUT, HIDDEN, or OUTPUT.
bool NeuralNetwork::VerifyType(NeuronGroup::Type type)
{
	int typei = (int)type;

	if (typei < (int)NeuronGroup::Type::INPUT || typei >(int)NeuronGroup::Type::OUTPUT)
		return false;
	else
		return true;
}

//Makes sure an identifier specifies a neuron group within allListGroups.
bool NeuralNetwork::VerifyIdentifier(NeuronGroup::Identifier *ident)
{
	if (!VerifyType(ident->type))
		return false;

	if (ident->index < 0 || ident->index >= (int)allListGroups[(int)ident->type]->size())
		return false;

	return true;
}

double NeuralNetwork::ExpDistance(const vector<double>& exp, const vector<double>& compare)
{
	double sum = 0.0;

	for (unsigned i = 0; i < exp.size(); i++)
		sum += pow(exp[i] - compare[i], 2.0);

	return sqrt(sum);
}

//Expensive computation of novelty score for a new experiences.
vector<shared_ptr<NeuralNetwork::DistanceDescription>> NeuralNetwork::ComputeNewDistances(shared_ptr<NoveltyBufferOccupant> occupant)
{
	if (noveltyBuffer.size() == 0)
		return vector<shared_ptr<NeuralNetwork::DistanceDescription>>();

	vector<shared_ptr<DistanceDescription>> distanceDescriptions = vector<shared_ptr<NeuralNetwork::DistanceDescription>>();

	if (noveltyBuffer.size() < historyBufferSize)
	{
		distanceDescriptions = vector<shared_ptr<DistanceDescription>>();

		for(auto currentOccupant : noveltyBuffer)
		{
			double distance = ExpDistance(occupant->experience, currentOccupant->experience);

			shared_ptr<DistanceDescription> newDescription = make_shared<DistanceDescription>();
			newDescription->distanceOwner = currentOccupant;
			newDescription->distance = distance;

			distanceDescriptions.push_back(newDescription);
		}
	}
	else
	{
		//Only need one less than the number of novelty buffer occupants.
		distanceDescriptions = vector<shared_ptr<DistanceDescription>>();

		//Don't include the least novel occupant in the distance cache.
		for (unsigned i = 1; i < noveltyBuffer.size(); i++)
		{
			shared_ptr<NoveltyBufferOccupant> currentOccupant = noveltyBuffer[i];

			double distance = ExpDistance(occupant->experience, currentOccupant->experience);

			shared_ptr<DistanceDescription> newDescription = make_shared<DistanceDescription>();
			newDescription->distanceOwner = currentOccupant;
			newDescription->distance = distance;

			distanceDescriptions.push_back(newDescription);
		}
	}

	//Copy the distances for sorting. The original list order is maintained 
	//so its distances can easily be added to the other novelty buffer occupants.
	vector<shared_ptr<DistanceDescription>> sortedList = vector<shared_ptr<DistanceDescription>>();

	for (unsigned i = 0; i < distanceDescriptions.size(); i++)
		sortedList.push_back(distanceDescriptions[i]);

	//sortedList.Sort();
	sort(sortedList.begin(), sortedList.end(), 
		[](const shared_ptr<DistanceDescription> & a, const shared_ptr<DistanceDescription> & b)
	{
		return *a < *b;
	});

	//Find out how many elements to keep in the occupant's distance caches.
	int keepCount = sortedList.size();
	//Find out how many elements to use for the occupant's novelty score.
	int nearestCount = sortedList.size();

	//keepCount should be at most N_KEEP.
	if (keepCount > (int)N_KEEP)
		keepCount = (int)N_KEEP;

	//nearestCount should be at most N_NEAREST.
	if (nearestCount > (int)N_NEAREST)
		nearestCount = (int)N_NEAREST;

	for (int i = 0; i < keepCount; i++)
		occupant->distanceDescriptions.push_back(sortedList[i]);

	for (int i = 0; i < nearestCount; i++)
		occupant->noveltyScore += sortedList[i]->distance;

	return distanceDescriptions;
}