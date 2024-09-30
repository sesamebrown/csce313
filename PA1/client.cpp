/*
    Author of the starter code
    Yifan Ren
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 9/15/2024
    
    Please include your Name, UIN, and the date below
    Name: Eden Kim
    UIN: 632008852
    Date: 9/29/2024
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <fstream>
#include <sys/wait.h>

using namespace std;


int main (int argc, char *argv[]) {
    int opt;
    int p = -1;
    double t = -1.0;
    int e = -1;
    string filename = "";
    int bufferCapacity = MAX_MESSAGE;
    bool new_channel = false;

    //Add other arguments here
    while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
        switch (opt) {
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                e = atoi (optarg);
                break;
            case 'f':
                filename = optarg;
                break;
            case 'm':
                bufferCapacity = atoi(optarg);
                break;
            case 'c':
                new_channel = true;
                break;
        }
    }

    //Task 1:
    //Run the server process as a child of the client process
    pid_t pid = fork();
    if (pid == 0) {  // Child process
        char *args[] = {(char*)"./server", (char*)"-m", (char*)to_string(bufferCapacity).c_str(), nullptr};
        execvp(args[0], args);
        exit(1);
    }

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);
    FIFORequestChannel* channel_to_use = &chan;

    //Task 4:
    //Request a new channel
    if (new_channel) {
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        chan.cwrite(&m, sizeof(MESSAGE_TYPE));
        char newChannelName[30];
        chan.cread(newChannelName, sizeof(newChannelName));
        channel_to_use = new FIFORequestChannel(newChannelName, FIFORequestChannel::CLIENT_SIDE);
    }

    if (mkdir("received", 0777) == -1) {
        if (errno != EEXIST) {
            exit(1);
        }
    }

    //Task 2:
    //Request data points
    if (filename.empty()) {
        if (p != -1 && t == -1.0 && e == -1) {
            ofstream outputFile("received/x1.csv");

            // if (!outputFile) {
            //     cerr << "Error: Unable to open x1.csv for writing." << endl;
            //     exit(1);
            // }

            for (int i = 0; i < 1000; i++) {
                double requestTime = i * 0.004;
                datamsg x(p, requestTime, 1);
                channel_to_use->cwrite(&x, sizeof(datamsg));
                double ecg1;
                channel_to_use->cread(&ecg1, sizeof(double));

                x.ecgno = 2;  // Request ECG 2
                channel_to_use->cwrite(&x, sizeof(datamsg));
                double ecg2;
                channel_to_use->cread(&ecg2, sizeof(double));

                outputFile << requestTime << "," << ecg1 << "," << ecg2 << endl;
            }
            outputFile.close();
            // cout << "Created x1.csv with 1000 data points for person " << p << endl;
        } else if (p != -1 && t != -1.0 && e != -1) { // for single data point
            char buf[MAX_MESSAGE];
            datamsg x(p, t, e);
            memcpy(buf, &x, sizeof(datamsg));
            channel_to_use->cwrite(buf, sizeof(datamsg));
            double reply;
            channel_to_use->cread(&reply, sizeof(double));
            cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
        }
    }

    //Task 3:
    //Request files
    if (!filename.empty()) {
        filemsg fm(0, 0);
        string fname = filename;
        
        int len = sizeof(filemsg) + (fname.size() + 1);
        char* buffer2 = new char[len];
        memcpy(buffer2, &fm, sizeof(filemsg));
        strcpy(buffer2 + sizeof(filemsg), fname.c_str());
        channel_to_use->cwrite(buffer2, len);

        __int64_t file_length;
        channel_to_use->cread(&file_length, sizeof(__int64_t));

        ofstream outputFile("received/" + filename, ios::binary);
        char* buffer = new char[bufferCapacity];
        __int64_t offset = 0;

        while (offset < file_length) {
            int length = min(bufferCapacity, static_cast<int>(file_length - offset));

            filemsg file_req(offset, length);
            memcpy(buffer2, &file_req, sizeof(filemsg));
            channel_to_use->cwrite(buffer2, len);
            
            int nbytes = channel_to_use->cread(buffer, length);
            outputFile.write(buffer, nbytes);
            offset += nbytes;
        }

        delete[] buffer;
        delete[] buffer2;
		
        outputFile.close();
    }
    
    //Task 5:
    // Closing all the channels
    MESSAGE_TYPE m = QUIT_MSG;
    channel_to_use->cwrite(&m, sizeof(MESSAGE_TYPE));
    if (new_channel) {
        delete channel_to_use;
    }
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));

    // Wait for the server to finish
    wait(nullptr);

    return 0;
}