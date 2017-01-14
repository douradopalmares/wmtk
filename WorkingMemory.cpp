//  Working Memory Toolkit (WMtk)
//  Adaptive Working Memory Library
//  Copyright (C) Joshua L. Phillips & Grayson M. Dubois
//  Department of Computer Science
//  Middle Tennessee State University; Murfreesboro, Tennessee, USA.

//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 3 of the License, or
//  (at your option) any later version

//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTIBILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.

//  You should have recieved a copy of the GNU General Public License
//  along with this program; if not, write to the Free Sotware
//  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA

/******************************************************************************
 * WorkingMemory.h
 *
 * This software is part of the Working Memory Toolkit.
 *
 * This class holds all of the details of the critic network which will be
 * used to approximate the value of storing or removing chunks from working
 * memory. It is the workhorse of a TD learning system.
 *
 * Author:    Grayson M. Dubois
 * Mentor:    Joshua L. Phillips
 ******************************************************************************/

#include <iostream>
#include <algorithm>
#include <utility>
#include <vector>
#include <random>
#include <math.h>
#include "WorkingMemory.h"
#include "hrr/hrrengine.h"
#include "hrr/hrrOperators.h"
#include "CriticNetwork.h"

using namespace std;

ostream& operator<<(ostream& os, const vector<string>& strings) {
  int x = 0;
  os << strings[0];
  for (x = 1; x < strings.size(); x++)
    os << "," << strings[x];
  return os;
}

/**---------------------------------------------------------------------------*
 *  CONSTRUCTORS AND INITIALIZERS
 *----------------------------------------------------------------------------*/

// Default Constructor
WorkingMemory::WorkingMemory() : re(1) {

    // Set number of working memory slots
    this->workingMemoryChunks.resize(3);

    // Set values for previous state
    this->previousReward = 0.0;
    this->previousValue = 0.0;

    // Set up the hrr engine and critic network
    this->vectorSize = 128;
    this->hrrengine.setVectorSize(this->vectorSize);
    this->critic.vectorSize = this->vectorSize;

    // Set up the eligibility trace
    this->eligibilityTrace.resize(this->vectorSize, 0);
    this->actionEligibilityTrace.resize(this->vectorSize, 0);

    // Instantiate the weight vector with small random values
    uniform_real_distribution<double> distribution(-0.01, 0.01);
    this->weights.resize(this->vectorSize);
    for ( int i = 0; i < this->vectorSize; i++ ) {
        this->weights[i] = distribution(re);
    }

    // Instantiate the action weight vector with small random values
    this->actionWeights.resize(this->vectorSize);
    for ( int i = 0; i < this->vectorSize; i++ ) {
        this->actionWeights[i] = distribution(re);
    }

    // Set up the random permutation vector
    for ( int i = 0; i < this->vectorSize; i++ ) {
        this->permutation.push_back(i);
    }
    shuffle(this->permutation.begin(), this->permutation.end(), this->re);
}

// Initializing Constructor
// Initializes Working Memory object with the given properties
WorkingMemory::WorkingMemory( double learningRate,
                              double discount,
                              double lambda,
                              double epsilon,
                              int vectorSize,
                              int numberOfChunks,
                              int seed) : re(seed), hrrengine(seed) {

    // Set number of working memory slots
    this->workingMemoryChunks.resize(numberOfChunks);

    // Set values for previous state
    this->previousReward = 0.0;
    this->previousValue = 0.0;

    // Set up the hrr engine and critic network
    this->vectorSize = vectorSize;
    this->hrrengine.setVectorSize(vectorSize);
    this->critic.setProperties(learningRate, discount, lambda, epsilon, vectorSize);

    // Set up the eligibility traces
    this->eligibilityTrace.resize(vectorSize, 0);
    this->actionEligibilityTrace.resize(vectorSize, 0);

    // Instantiate the weight vector with small random values
    uniform_real_distribution<double> distribution(-0.01, 0.01);
    this->weights.resize(this->vectorSize);
    for ( int i = 0; i < this->vectorSize; i++ ) {
        this->weights[i] = distribution(re);
    }

    // Instantiate the action weight vector with small random values
    this->actionWeights.resize(vectorSize);
    for ( int i = 0; i < vectorSize; i++ ) {
        this->actionWeights[i] = distribution(re);
    }

    // Set up the random permutation vector
    for ( int i = 0; i < vectorSize; i++ ) {
        this->permutation.push_back(i);
    }
    shuffle(this->permutation.begin(), this->permutation.end(), this->re);
    return;
}

// Copy-Constructor
WorkingMemory::WorkingMemory(const WorkingMemory& rhs) {

    this->critic = rhs.critic;
    this->hrrengine = rhs.hrrengine;
    this->re = rhs.re;

    for ( string chunk : rhs.workingMemoryChunks ) {
        this->workingMemoryChunks.push_back(chunk);
    }

    this->vectorSize = rhs.vectorSize;

    this->previousReward = rhs.previousReward;
    this->previousValue = rhs.previousValue;

    this->eligibilityTrace.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->eligibilityTrace[i] = rhs.eligibilityTrace[i];
    }

    this->actionEligibilityTrace.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->actionEligibilityTrace[i] = rhs.actionEligibilityTrace[i];
    }

    this->weights.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->weights[i] = rhs.weights[i];
    }

    this->actionWeights.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->actionWeights[i] = rhs.actionWeights[i];
    }

    this->permutation.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->permutation[i] = rhs.permutation[i];
    }
}

// Destructor
WorkingMemory::~WorkingMemory() {}

// Assignment Operator
WorkingMemory& WorkingMemory::operator=(const WorkingMemory& rhs) {

    this->critic = rhs.critic;
    this->hrrengine = rhs.hrrengine;
    this->re = rhs.re;

    for ( string chunk : rhs.workingMemoryChunks ) {
        this->workingMemoryChunks.push_back(chunk);
    }

    this->vectorSize = rhs.vectorSize;

    this->previousReward = rhs.previousReward;
    this->previousValue = rhs.previousValue;

    this->eligibilityTrace.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->eligibilityTrace[i] = rhs.eligibilityTrace[i];
    }

    this->actionEligibilityTrace.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->actionEligibilityTrace[i] = rhs.actionEligibilityTrace[i];
    }

    this->weights.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->weights[i] = rhs.weights[i];
    }

    this->actionWeights.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->actionWeights[i] = rhs.actionWeights[i];
    }

    this->permutation.resize(rhs.vectorSize);
    for ( int i = 0; i < rhs.vectorSize; i++ ) {
        this->permutation[i] = rhs.permutation[i];
    }

    return *this;
}


/**---------------------------------------------------------------------------*
 *  (*MAIN DEVELOPER INTERFACE*) LEARNING PROCESS METHODS
 *----------------------------------------------------------------------------*/

// Initialize the episode.
//  Takes the string representation of the initial state and an optional
//  value for the reward at that state (typically 0). Sets the episode up.

// The value returned is the recommended best action for the state based on the Q function
string WorkingMemory::initializeEpisode(string state, vector<string> possibleActions, double reward) {

    /**
    *  Choose Working Memory Contents
    */

    // Store the current state
    this->state = state;

    // Clear working memory
    fill(workingMemoryChunks.begin(),workingMemoryChunks.end(),"I");

    // Get the candidate chunks from the state
    vector<string> candidateChunks = getCandidateChunksFromState();
    sort(candidateChunks.begin(),candidateChunks.end());

    // Find the most valuable chunks and store in working memory,
    // or random under the Epsilon Soft policy
    uniform_real_distribution<double> distribution(0.0, 1.0);
    if ( distribution(re) < critic.epsilon ) {
        chooseRandomWorkingMemoryContents(candidateChunks);
    } else {
        findMostValuableChunks(candidateChunks);
    }

    /**
    *  Calculate WM Value
    */

    // Get the representation of the state and current working memory contents
    // Find its value
    HRR representation = stateAndWorkingMemoryRepresentation();
    double value = critic.V(representation, weights);

    /**
    *  Choose Action
    */

    // Use Q function to pick the best action for the new state/WM combo
    // Convolve action with current state and working memory contents
    pair<string,HRR> action = findMostValuableAction(possibleActions);

    /**
    *  Calculate Action Value
    */

    double qValue = critic.V(action.second, actionWeights);

    /**
    *  Store t-1 State Information
    */

    // Store the value and reward for use in the next step
    previousStateWorkingMemory = representation;
    previousValue = value;
    previousReward = reward;
    previousStateWorkingMemoryAction = action.second;
    previousQValue = qValue;

    /**
    *  Clear WM Eligibility Trace
    */

    fill(eligibilityTrace.begin(), eligibilityTrace.end(), 0.0);

    /**
    *  Clear Action Eligibility Trace
    */

    fill(actionEligibilityTrace.begin(), actionEligibilityTrace.end(), 0.0);

    return action.first;
}

// Take a step in the episode.
//  Takes the string representation of the current state and an optional
//  value for the reward at that state (typically 0). Calculates a guess of
//  what information is most valuable to retain from current state.

// The state variable is for time t+1, action is for time t, reward is for time t

// The value update being performed here (using variable names) is:
//   V(previousStateWorkingMemory) = V(previousStateWorkingMemory) + 
//   alpha*[reward + gamma*V(representation) - V(previousStateWorkingMemory)]

// The Q update being performed here (using variable names) is:
//   Q(previousStateWorkingMemory,previousAction) = Q(previousStateWorkingMemory,previousAction) +
//   alpha*[reward + gamma*Q(representation,action) - Q(previousStateWorkingMemory,previousAction)]

// The value returned is the recommended best action for the state based on the Q function
// We are assuming that this action will always be taken; otherwise the learning will not work
string WorkingMemory::step(string state, vector<string> possibleActions, double reward) {

    /**
    *  Update WM eligibility trace
    */

    for (int x = 0; x < vectorSize; x++) {
        eligibilityTrace[x] *= critic.lambda;
        eligibilityTrace[x] += (previousStateWorkingMemory[x] / sqrt(2));
    }

    /**
    *  Update Action eligibility trace
    */

    for (int x = 0; x < vectorSize; x++) {
        actionEligibilityTrace[x] *= critic.lambda;
        actionEligibilityTrace[x] += (previousStateWorkingMemoryAction[x] / sqrt(2));
    }

    /**
    *  Choose Working Memory Contents
    */

    // Store the current state
    this->state = state;

    // Build the list of candidate chunks
    vector<string> candidateChunks = getCandidateChunksFromState();

    // Add current working memory contents to the list of candidates, as long as they are not already there
    for ( string chunk : workingMemoryChunks ) {
      if ( chunk != "I" && find( candidateChunks.begin(), candidateChunks.end(), chunk ) == candidateChunks.end() ) {
	candidateChunks.push_back(chunk);
      }
    }
    sort(candidateChunks.begin(),candidateChunks.end());

    // Find the most valuable chunks and store in working memory,
    // or random under the Epsilon Soft policy
    uniform_real_distribution<double> distribution(0.0, 1.0);
    if ( distribution(re) < critic.epsilon ) {
        chooseRandomWorkingMemoryContents(candidateChunks);
    } else {
        findMostValuableChunks(candidateChunks);
    }

    /**
    *  Calculate WM Value
    */

    // Get the representation of the state and current working memory contents
    // Find its value
    HRR representation = stateAndWorkingMemoryRepresentation();
    double value = critic.V(representation, weights);

    /**
    *  Choose Action
    */

    // Use Q function to pick the best action for the new state/WM combo
    // Convolve action with current state and working memory contents
    pair<string,HRR> action = findMostValuableAction(possibleActions);

    /**
    *  Calculate Action Value
    */

    double qValue = critic.V(action.second, actionWeights);

    /**
    *  Perform TD learning function for WM
    */

    // Find the TDError between the current state and the previous state
    double error = critic.TDError(previousReward, value, previousValue);

    // Update the weight vector
    for ( int x = 0; x < vectorSize; x++ ) {
        weights[x] += critic.alpha * error * eligibilityTrace[x];
    }

    /**
    *  Perform TD learning function for Action
    */

    // Find the TDError between the current state and the previous state
    error = critic.TDError(previousReward, qValue, previousQValue);

    // Update the weight vector
    for ( int x = 0; x < vectorSize; x++ ) {
        actionWeights[x] += critic.alpha * error * actionEligibilityTrace[x];
    }

    /**
    *  Store t-1 State Information
    */

    // Store the value and reward for use in the next step
    previousStateWorkingMemory = representation;
    previousValue = value;
    previousReward = reward;
    previousStateWorkingMemoryAction = action.second;
    previousQValue = qValue;

    return action.first;
}

// Get the final reward and finish the episode.
void WorkingMemory::absorbReward(double reward) {

    /**
    *  Update WM eligibility trace
    */

    for (int x = 0; x < vectorSize; x++) {
        eligibilityTrace[x] *= critic.lambda;
        eligibilityTrace[x] += (previousStateWorkingMemory[x] / sqrt(2));
    }

    /**
    *  Update Action eligibility trace
    */

    for (int x = 0; x < vectorSize; x++) {
        actionEligibilityTrace[x] *= critic.lambda;
        actionEligibilityTrace[x] += (previousStateWorkingMemoryAction[x] / sqrt(2));
    }

    /**
    *  Perform TD learning function for WM at Goal State
    */

    // Get the value for the goal state
    double error = critic.TDError(reward, previousValue);

    // Update the weight vector for the goal
    for ( int x = 0; x < vectorSize; x++ ) {
        weights[x] += critic.alpha * error * eligibilityTrace[x];
    }

    /**
    *  Perform TD learning function for Action at Goal State
    */

    // Get the value for the goal state
    error = critic.TDError(reward, previousQValue);

    // Update the weight vector for the goal
    for ( int x = 0; x < vectorSize; x++ ) {
        actionWeights[x] += critic.alpha * error * actionEligibilityTrace[x];
    }
}

// Get all chunks currently held in working memory
vector<string> WorkingMemory::queryWorkingMemory() {

    vector<string> chunkNames;

    for (string chunk : workingMemoryChunks) {
        chunkNames.push_back(chunk);
    }

    return chunkNames;
}

// Get chunk in working memory at specific index
string WorkingMemory::queryWorkingMemory(int atIndex) {
    return workingMemoryChunks[atIndex];
}


// Clear the weight vector
void WorkingMemory::clearWeights() {
    fill(weights.begin(), weights.end(), 0.0);
}

// Reset the weight vector to small random values between -0.01 and 0.01
void WorkingMemory::resetWeights() {
    uniform_real_distribution<double> distribution(-0.01, 0.01);
    for ( int i = 0; i < vectorSize; i++ ) {
        weights[i] = distribution(re);
    }
}

// Reset the weight vector to random values in the specified range
void WorkingMemory::resetWeights(double lower, double upper) {
    uniform_real_distribution<double> distribution(lower, upper);
    for ( int i = 0; i < vectorSize; i++ ) {
        weights[i] = distribution(re);
    }
}


/**---------------------------------------------------------------------------*
 *  HELPER METHODS
 *----------------------------------------------------------------------------*/

 // Unpack the state into a vector of possible candidates for working memory
vector<string> WorkingMemory::getCandidateChunksFromState() {

    vector<string> candidateChunks;

    // First, we separate the concepts represented in the state by splitting on
    //  the '+' character
    vector<string> stateConceptNames = HRREngine::explode(state, '+');

    // Now we check if there is an empty string in the list, and remove it
    stateConceptNames.erase( remove( stateConceptNames.begin(), stateConceptNames.end(), "I" ), stateConceptNames.end() );

    // Add the combinations of constituent concepts for each state concept to
    //  the list of candidate chunks
    vector<string>::iterator iter;
    for (string concept : stateConceptNames) {
         vector<string> candidates = candidateChunks;

         vector<string> unpackedConcepts = hrrengine.unpackSimple(concept);

         candidateChunks.resize( candidates.size() + unpackedConcepts.size());

         // Finds the union of the current candidate chunks with the newly
         //  unpacked concepts, and stores in the candidate chunks vector
         iter = set_union( candidates.begin(), candidates.end(),
                    unpackedConcepts.begin(), unpackedConcepts.end(),
                    candidateChunks.begin() );
    }

    return candidateChunks;
}

 // Collects a random selection of candidateChunks to put in working memory
void WorkingMemory::chooseRandomWorkingMemoryContents(vector<string> candidates) {

  for (int i = 0; i < workingMemoryChunks.size(); i++) {
    if (candidates.size() == 0) {
      workingMemoryChunks[i] = "I";
    }
    else {
      uniform_int_distribution<int> distribution(-1,candidates.size()-1);
      int chunk = distribution(re);
      if (chunk > 0) {
	workingMemoryChunks[i] = candidates[chunk];
	sort(workingMemoryChunks.begin(),workingMemoryChunks.begin()+i);
	candidates.erase(candidates.begin() + chunk, candidates.begin() + chunk + 1);
      }
      else {
	workingMemoryChunks[i] = "I";
	while (!candidates.empty())
	  candidates.pop_back();
      }
    }
  }
}

// Compare all possible combinations of candidate chunks with the
void WorkingMemory::findMostValuableChunks(vector<string> candidateChunks) {

  int n = candidateChunks.size();
  int r = workingMemoryChunks.size();
  if (r > n)
    r = n;

  vector<string> combination(workingMemoryChunks.size());
  fill(combination.begin(),combination.end(),"I");

  workingMemoryChunks = combination;
  currentChunkValue = findValueOfContents(combination);

  for (int x = 1; x <= r; x++) {
    vector<bool> v(n);
    fill(v.begin(), v.begin() + x, true);

    do {
      int fill = 0;
      for (int i = 0; i < n; ++i) {
	if (v[i]) {
	  combination[fill++] = candidateChunks[i];
	}
      }

      double valueOfContents = findValueOfContents(combination);

      if ( valueOfContents > currentChunkValue ) {
	workingMemoryChunks = combination;
	currentChunkValue = valueOfContents;
      }

    } while (prev_permutation(v.begin(), v.end()));
  }

  return;
}

void WorkingMemory::findCombinationsOfCandidates(int offset, int slots, vector<string>& candidates, vector<string>& combination) {

    if (slots == 0) {
        double valueOfContents = findValueOfContents(combination);

        if ( valueOfContents >= currentChunkValue ) {
            workingMemoryChunks = combination;
            currentChunkValue = valueOfContents;
        }

        return;
    }

    // Temporary rewrite of below
    for (int i = 0; i < offset+1; i++) {
      combination.push_back(candidates[i]);
      findCombinationsOfCandidates(offset+i, slots-1, candidates, combination);
      combination.pop_back();
    }
}

// Find the HRR representing the state
HRR WorkingMemory::stateRepresentation() {

    vector<string> stateConceptNames = HRREngine::explode(state, '+');
    HRR stateRepresentation = hrrengine.query(stateConceptNames[0]);

    for ( int i = 1; i < stateConceptNames.size(); i++) {
        stateRepresentation = stateRepresentation + hrrengine.query(stateConceptNames[i]);
    }

    return stateRepresentation;
}

// Get the representation of the current working memory contents with the current state
HRR WorkingMemory::stateAndWorkingMemoryRepresentation() {

    // Get the convolved product of each chunk in working memory
    HRR representation = hrrengine.query(workingMemoryChunks[0]);
    for ( int i = 1; i < workingMemoryChunks.size(); i++ ) {
        representation = hrrengine.convolveHRRs(representation, hrrengine.query(workingMemoryChunks[i]));
    }

    // Permute the representation of
    if (hrrengine.dot(hrrengine.query("I"),representation) != 1.0)
      representation = permute(representation);

    // Convolve the representation of the WM contents with the state representation
    representation = hrrengine.convolveHRRs(representation,stateRepresentation());

    return representation;
}

// Calculate the value of the current state
double WorkingMemory::findValueOfState() {
    return critic.V(stateRepresentation(), weights);
}

// Calculate the value of a given state
double WorkingMemory::findValueOfState(vector<string> state) {

    // State values are simply added
    HRR state_representation = hrrengine.query(state[0]);
    for ( int i = 1; i < state.size(); i++) {
        state_representation = state_representation + hrrengine.query(state[i]);
    }

    return critic.V(state_representation, weights);
}

// MJ: added this so that we can find the value of the next possible states and pick the best one
// Calculate the value of a given state using the current working memory contents
double WorkingMemory::findValueOfStateWM(vector<string> state) {
    
    // Get the convolved product of each chunk in working memory
    HRR representation = hrrengine.query(workingMemoryChunks[0]);
    for ( int i = 1; i < workingMemoryChunks.size(); i++ ) {
        representation = hrrengine.convolveHRRs(representation, hrrengine.query(workingMemoryChunks[i]));
    }

    // Permute the working memory representation
    if (hrrengine.dot(hrrengine.query("I"),representation) != 1.0)
      representation = permute(representation);

    // State values are simply added
    HRR state_representation = hrrengine.query(state[0]);
    for ( int i = 1; i < state.size(); i++) {
        state_representation = state_representation + hrrengine.query(state[i]);
    }

    // Convolve the representation of the WM contents with the state representation
    representation = hrrengine.convolveHRRs(representation,state_representation);

    return critic.V(representation, weights);
}

// Calculate the value of a given set of working memory contents and state
double WorkingMemory::findValueOfContents(vector<string> contents) {

    // Get the convolved product of each chunk in working memory
    HRR representation = hrrengine.query(contents[0]);
    for ( int i = 1; i < contents.size(); i++ ) {
        representation = hrrengine.convolveHRRs(representation, hrrengine.query(contents[i]));
    }

    // Permute the internal representation of working memory contents
    if (hrrengine.dot(hrrengine.query("I"),representation) != 1.0)
        representation = permute(representation);

    // Convolve the representation of the WM contents with the state representation
    representation = hrrengine.convolveHRRs(representation,stateRepresentation());

    // Calculate the value of the representation of the current state and contents
    return critic.V(representation, weights);
}

// This takes a list of possible actions and a given convolutino of state/WM contents
// and picks the most valuable action to perform
pair<string,HRR> WorkingMemory::findMostValuableAction(vector<string> possibleActions) {
    string action;
    HRR actionHRR;
    double bestValue = -999;
    HRR stateWMRepresentation = stateAndWorkingMemoryRepresentation();

    for ( int i = 0; i < possibleActions.size(); i++ ) {

        HRR actionRepresentation = hrrengine.convolveHRRs(hrrengine.query(possibleActions[i]),stateWMRepresentation);
        double val = critic.V(actionRepresentation, actionWeights);

        if( val > bestValue )
        {
            action = possibleActions[i];
            actionHRR = actionRepresentation;
            bestValue = val;
        }
    }

    return make_pair(action,actionHRR);
}

// MJ: currently only used for debugging
// Calculate the value of a given set of working memory contents and state
double WorkingMemory::findValueOfStateContents(vector<string> state, vector<string> contents) {

    // Get the convolved product of each chunk in working memory
    HRR contents_representation = hrrengine.query(contents[0]);
    for ( int i = 1; i < contents.size(); i++ ) {
        contents_representation = hrrengine.convolveHRRs(contents_representation, hrrengine.query(contents[i]));
    }

    // Permute the internal representation of working memory contents
    if (hrrengine.dot(hrrengine.query("I"),contents_representation) != 1.0)
        contents_representation = permute(contents_representation);

    // State values are simply added
    HRR state_representation = hrrengine.query(state[0]);
    for ( int i = 1; i < state.size(); i++) {
        state_representation = state_representation + hrrengine.query(state[i]);
    }

    // Convolve the representation of the WM contents with the state representation
    contents_representation = hrrengine.convolveHRRs(contents_representation,state_representation);

    // Calculate the value of the representation of the current state and contents
    return critic.V(contents_representation, weights);
}

// Perform a permutation on an HRR
HRR WorkingMemory::permute(HRR original) {

    HRR permutedHRR(vectorSize);

    for ( int i = 1; i < vectorSize; i++ ) {
        permutedHRR[i] = original[ permutation[i] ];
    }

    return permutedHRR;
}

// Undo the permutation to find the original unshuffled HRR
HRR WorkingMemory::inversePermute(HRR permuted) {

    HRR originalHRR(vectorSize);

    for ( int i = 1; i < vectorSize; i++ ) {
        originalHRR[ permutation[i] ] = permuted[i];
    }

    return originalHRR;
}
void WorkingMemory::printWMContents()
{
    cout << "WM Contents:" << endl;
    for( string chunk : queryWorkingMemory() )
        cout << chunk << " | ";
    cout << endl;
}

