/*
Author's name: Kiana Kabiri
ID:9871304
date:24/11/2019 */


//pre-processor directives:
#include <iostream>
#include <thread>
#include <random>		//from notes for random number generator
#include <chrono>		//from notes for random number generator
#include <mutex>
#include <map>
#include <vector>
#include <condition_variable> 


using namespace std;


//global constants:
int const MAX_NUM_OF_CHAN = 6;	//number of AdcInputChannels 
int const MAX_NUM_OF_THREADS = 6;
int const DATA_BLOCK_SIZE = 20; //for A2 only
int const NUM_OF_LINKS = 3; //number of communication links in link access

//global variables:
std::condition_variable cond; // setting up the conditon variable 
std::map <std::thread::id, int> threadIDs;  //

//function prototypes: (if used)
int search(std::thread::id thread_IDs); //function prototype for the mapping function


// class for the input
class AdcInputChannel {
public:
  AdcInputChannel (int d)	         //Constructors intitializes 
  : currentSample(d) {}				//constructor syntax for 'currentSample=d'

  //used to request a sample from the sample channel:
  double getCurrentSample () {
  return currentSample;
      
  }
private:
  int currentSample;
};				//end class AdcInputChannel


class Lock {
public:
Lock() //constructor
 : open(true) {}
 
//returns a flag to indicate when a thread should be blocked:
    bool lock() {
    if(open){
        open = false;
        return true;
    }
    else{
        return false;
    }
}
void unlock() {
    open=true;
}
 private:
bool open;
}; //end class Lock

class ADC {
public:
//constructor: initialises a vector of ADCInputChannels
//passed in by reference:
  ADC (std::vector < AdcInputChannel > &channels)
    : adcChannels(channels) {}

  void requestADC (int c) {	
        if(!theADCLock.lock()) {      //starting an if statement for when the ADC is locked
            std::unique_lock<mutex> locker(ADC_mu);
            cout << "ADC is currently locked, so thread " << search(std::this_thread::get_id()) << " is suspended." << endl;
            cond.wait(locker);  //makes thread wait when ADC is being used
    }
    sampleChannel = c;                                        
    cout<<theADCLock.lock()<<endl;                                        //Lock the ADC
	std::unique_lock<mutex> locker(ADC_mu);                   //lock thread to print
    cout << "ADC is locked, thread " << search(std::this_thread::get_id()) << " currently accessing the ADC" << endl;
  }
  
    double sampleADC () {
      
      std::unique_lock<mutex> locker(ADC_mu); 
      cout << "ADC sample by thread " << search(std::this_thread::get_id())  << " is " << sampleChannel*2 << endl;
    return sampleChannel*2;
        
    }
  
     void releaseADC () {
         
    theADCLock.unlock();
     std::unique_lock<mutex> locker(ADC_mu);
     cout << "ADC is released from thread " <<search(std::this_thread::get_id()) <<endl;
     cond.notify_one();       //notifies thread that it ADC is released and it can carry on
    }
  
private:
  Lock theADCLock;
  int sampleChannel;
  std::vector < AdcInputChannel > &adcChannels;	//vector reference
  std::mutex ADC_mu;		//declaring an ADC mutex
};				//end class ADC


    int search(std::thread::id thread_IDs)  {                                //opening the mapping function
    std::map <std::thread::id, int>::iterator it
                = threadIDs.find(std::this_thread::get_id());
    if (it == threadIDs.end()) return -1; //thread 'id' NOT found
    else return it->second; //thread 'id' found, return the
                           //associated integer –note the syntax.
}

class Receiver {
 public:
 //constructor:
 Receiver () { //syntax -no initialiser list, no ":"
 //initialise dataBlocks:
     for (int i = 0; i < MAX_NUM_OF_THREADS; i++) {
         for (int j = 0; j < DATA_BLOCK_SIZE; j++) {
             dataBlocks[i][j] = 0;
            }
        }
    }
 //Receives a block of doubles such that the data
//is stored in index id of dataBlocks[][]
 void receiveDataBlock(int id, double data[]) {
                for(int i = 0; i < DATA_BLOCK_SIZE; i++){           //Thread id is the row for the data array
                    dataBlocks[id][i] = data[i];
 }
 }
 // print out all items

void printBlocks() {
                    for (int i = 0; i < MAX_NUM_OF_THREADS; i++) {
                            for (int j = 0; j < DATA_BLOCK_SIZE; j++) {
                                cout << dataBlocks[i][j] <<" ";
                            }
                    cout << endl;
                    }
}
 private:
 double dataBlocks[MAX_NUM_OF_THREADS][DATA_BLOCK_SIZE];

}; //end class Receiver

class Link {
 public:
 Link (Receiver& r, int linkNum) //Constructor
 : inUse(false), myReceiver(r), linkId(linkNum)
 {}
 //check if the link is currently in use
 bool isInUse() {
 return inUse;
 }
 
 //set the link status to busy
 void setInUse() {
 inUse = true;
 }
 
 //set the link status to idle
 void setIdle() {
 inUse = false;
 }
 
 //write data[] to the receiver
 void writeToDataLink(int id, double data[]) {
 myReceiver.receiveDataBlock(id, data);
}

 //returns the link Id
 int getLinkId() {
 return linkId;
 }
 
 private:
 bool inUse;
 Receiver& myReceiver; //Receiver reference
 int linkId;
}; //end class Link



class LinkAccessController {
 public:
 LinkAccessController(Receiver& r) //Constructor
 : myReceiver(r), numOfAvailableLinks(NUM_OF_LINKS)
 {
 for (int i = 0; i < NUM_OF_LINKS; i++) {
 commsLinks.push_back(Link(myReceiver, i));
 }
 }
 //Request a comm's link: returns an available Link.
 //If none are available, the calling thread is suspended.
 Link requestLink() {
            if(numOfAvailableLinks ==  0){                             //pause the thread if there is no free links
                    std::unique_lock <mutex> lk(LAC_mu);
                    cond.wait(lk);
                
            } //remember that if the code doesnt work you might have to change it to lck
                    
                    for(linkNum = 0;linkNum  < NUM_OF_LINKS-1; linkNum++){
                        if(!commsLinks[linkNum].isInUse())
                break;                                                         //try next link for availability
                }
        commsLinks[linkNum].setInUse();
        numOfAvailableLinks--;   
 return commsLinks[linkNum];
 }
 //Release a comms link:
 void releaseLink(Link& releasedLink) {
                releasedLink.setIdle();
                numOfAvailableLinks++;
                cond.notify_one();                                      

 }

 private:
 Receiver& myReceiver; //Receiver reference
 int numOfAvailableLinks;
 int linkNum;
 std::vector<Link> commsLinks;
 std::mutex LAC_mu; //mutex
}; //end class LinkAccessController








//run function -executed by each thread:


void
run (ADC & theADC, int id, LinkAccessController & lac)
{
    //mapping thread
    std::mutex mu; //declare a mutex
     std::unique_lock<std::mutex> map_locker(mu); //lock the map via the mutex.
    //insert the threadID and id into the map:
    threadIDs.insert(std::make_pair(std::this_thread::get_id(), id));
     map_locker.unlock(); //we're done, unlock the map.

	//to store the sampled data: (for part A2 only)
	double sampleBlock[DATA_BLOCK_SIZE] = {0.0};  //initialise all elements to 0
	for (int i = 0; i<DATA_BLOCK_SIZE; i++) { //replace with 'i<DATA_BLOCK_SIZE;' for A2
// request use of the ADC; channel to sample is id:
    theADC.requestADC(id);
      
// sample the ADC:
	sampleBlock[i] = theADC.sampleADC();
	
// release the ADC:
	theADC.releaseADC();
	
// delay for random period between 0.1s b 0.5s:
      std::random_device rd;  //Will be used to obtain a seed for the random number engine
	  //time(0) gives the number of seconds elapsed since the start
      std::mt19937 gen (time (0));	//A 'Mersenne_twister_engine' seeded by time(0).
//of the world according to Unix (00:00:00 on 1st January 1970).
      std::uniform_int_distribution <> dis (100, 500);	//generate a random integer
//between 1-1000.
      int n = dis (gen);	//-the pseudo-random number is stored in n.
      std::this_thread::sleep_for(std::chrono::milliseconds(n)); //Delay for a random time between 0.1 and 0.5 second.
    }
    	Link link = lac.requestLink();                                   
    link.writeToDataLink(search(std::this_thread::get_id()), sampleBlock);
    lac.releaseLink(link);
    cout << "Releasing link"<< search(std::this_thread::get_id()) << endl;
}


int main ()
{
  //initialise the ADC channels:
  std::vector < AdcInputChannel > adcChannels;
  
  for (int i = 0; i < MAX_NUM_OF_CHAN; i++)
    {				//for loop
      adcChannels.push_back(AdcInputChannel(i));  		//each AdcInputChannel is initialised with a different value as part of the vector
    }

  //instantiate the ADC:
  ADC adc(adcChannels);

 // Instantiate the Receiver:
Receiver receiver;
 // Instantiate the LinkAccessController:
    LinkAccessController lac(receiver);
 
    //instantiate and start the threads:
    std::thread the_threads[MAX_NUM_OF_THREADS];	//array of threads
    
  for (int i = 0; i < MAX_NUM_OF_THREADS; i++)
    {				//for loop

      //launch the threads:
    	the_threads[i]=std::thread(run,std::ref(adc),i, std::ref(lac)) ; //reference to ADC Class
  }


  //wait for the threads to finish:
    for (int i = 0; i < MAX_NUM_OF_THREADS; i++)
    {
      the_threads[i].join ();
    }
    
     // Print out the data in the Receiver:
  receiver.printBlocks();

    
  cout << "All threads terminated" << endl;

  return 0;
}







