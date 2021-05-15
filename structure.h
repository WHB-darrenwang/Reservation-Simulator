#include <string>
#include <iostream>
#include <mutex>

std::mutex cout_lock;

static const int 	MAX_MESSAGE_SIZE 	= 256;		// Does not include the last @
static const size_t NUM_CREATE_PARAM 	= 4;

// Ports
static const int 	MASTER_WORKER_PORT 	= 8080;
static const int 	HEARTBEAT_PORT 		= 8888;


void paste(const std::string& str){
    cout_lock.lock();
    std::cout << str << "\n";
    cout_lock.unlock();
}

class location{
private:
	std::string name;
	unsigned int longitude;
	unsigned int latitude;
	std::string filename;
	unsigned int rating;
public:
	location(std::string& name_, 
            unsigned int longitude_,
            unsigned int latitude_,
            std::string filename_):
            name(name_),
            longitude(longitude_),
            latitude(latitude_),
            filename(filename_){
        paste("Created a new location");
	}

	// getter functions
	std::string getName(){
		return name;
	}

	unsigned int getLongitude(){
		return longitude;
	}

	unsigned int getLatitude(){
		return latitude;
	}

	std::string getFilename(){
		return filename;
	}

	unsigned int getRating(){
		return rating;
	}

	// setter functions
	void setName(std::string & input){
		name = input;
	}

	void getLongitude(unsigned int input){
		longitude = input;
	}

	void getLatitude(unsigned int input){
		latitude = input;
	}

	void getFilename(std::string & input){
		filename = input;
	}

	void getRating(unsigned int input){
		rating = input;
	}
};