//
// Created by Taher Jamali on 4/9/21.
//
#include <iostream>
#include <fstream>
#include <vector>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <string.h>
#include <sys/wait.h>
#include <signal.h> // might not need this (use to kill process)

using namespace std;

char error_message[30] = "An error has occurred\n";  // Global error message

vector<vector<string> > parseData(string input); // will be used to parse the data and store it in a vector of strings
string removeExtraSpaces(string basicString); // helper function for parseData that will remove extra spaces... doesn't work properly
int forkHere(vector<string> command, vector<string> &path); // will fork the process and execute the command given in
// the vector
void commandHandler(vector<vector<string> > parseCom, vector<string> &path); // function that checks for built-in function
// or other functions
string validPath(vector<string> command, vector<string> &path); // will get the path vector and check for the first valid path. if none are
// found then return empty string

int validRedirect(vector<string> &command); // will go through command and check for > and handle redirection

int main(int argc, char *argv[]) {
    string command; // command is a string and will be used to store the unparsed input from user or file
    vector<vector<string> > parseCom; // will be used to store the parsed commands from user or file
    ifstream file; // will be used in batch mode to open files and read input
    vector<string> path; // vector to hold all the paths the user might input, path is initialized to /bin/
    path.push_back("/bin/");

    // start while loop
    while(strcmp(command.c_str(),"exit") != 0 || file.eof()){ // runs until user types exit command
        // interactive mode, has the prompt "wish> " and use getline()
        if(argc == 1){
            cout << "wish> ";
            getline(cin,command);
            // if exit command was typed, break from while loop
            if(strcmp(command.c_str(),"exit") == 0){
                //cout << "Exit was typed" << endl;
                exit(0);
            }
            else{ // command is not exit
                // parse the input given
                parseCom = parseData(command);
                // call function that checks for built-in function or other functions
                commandHandler(parseCom, path);
            }
        }
        else{ // batch mode, read from file and use getline()
            // open the file from the argument
            // cout << "Program is in batch mode" << endl;
            file.open(argv[1]);
            // check if the file was opened or is empty. if it is either send error message
            if(!file.is_open() || file.peek() == std::ifstream::traits_type::eof()){
                write(STDERR_FILENO, error_message, strlen(error_message));
                // exit program with 1
                exit(1);
            }
            else{
                // read from the file line by line
                while(getline(file, command)){ // need to also check for EOF and exit
                    // parse command and store into vector
                    //cout << "Command is " << command << endl;
                    if(strcmp(command.c_str(),"exit") == 0){
                        //cout << "Program is in batch mode" << endl;
                        break;
                    }
                    parseCom = parseData(command);
                    // call function that checks for built-in function or other functions
                    commandHandler(parseCom, path);
                }
                if(file.eof()){ // file has been all read so now program will exit
                    break;
                }
            }
        }
    }
    // exit
    exit(0);
    //return 0;
}

void commandHandler(vector<vector<string> > parseCom, vector<string> &path) {
    pid_t forked;
    // for loop to go through the commands
    for(unsigned int idx = 0; idx < parseCom.size(); idx++){
        vector<string> oneCommand = parseCom[idx];
        // see if any of the built-in commands are in parseCom
        if(oneCommand[0] == "cd"){
            int errorMes;
            // cd only takes one argument so if size of parseCom is > 2, program will error
            if(oneCommand.size() != 2){
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
            else{
                // chdir should not return -1. that causes error
                errorMes = chdir(oneCommand[1].c_str());
                if(errorMes < 0){
                    write(STDERR_FILENO, error_message, strlen(error_message));
                }
            }
        }
        else if(oneCommand[0] == "path"){ // when path has no arguments program needs to clear so that exec can't run
            // clear path vector
            path.clear();
            //cout << "parseCom.size() is " << parseCom.size() << endl;
            // add what ever is after path to the string vector
            for(unsigned int i = 1; i < oneCommand.size(); i++){
                string temp = oneCommand[i] + "/";
                path.push_back(temp);
            }
//        for(int i = 0; i < path.size();i++){
//            cout << "path" << i << " is " << path[i] << endl;
//        }
        }
        else{ // built in commands were not used so program will fork
            // call forkHere to handle the fork
            forked = forkHere(parseCom[idx], path);
        }
    }

    if(forked != 0){ // in parent process
        int returnStatus;
        waitpid(forked, &returnStatus, 0);
    }
}

string removeExtraSpaces(string basicString) {
    // n is length of the original string
    int n = basicString.length();

    // i points to next position to be filled in
    // output string/ j points to next character
    // in the original string
    int i = 0, j = -1;

    // flag that sets to true is space is found
    bool spaceFound = false;

    // Handles leading spaces
    while (++j < n && basicString[j] == ' ');

    // read all characters of original string
    while (j < n)
    {
        // if current characters is non-space
        if (basicString[j] != ' ')
        {
            // copy current character at index i
            // and increment both i and j
            basicString[i++] = basicString[j++];

            // set space flag to false when any
            // non-space character is found
            spaceFound = false;
        }
            // if current character is a space
        else if (basicString[j++] == ' ')
        {
            // If space is encountered for the first
            // time after a word, put one space in the
            // output and set space flag to true
            if (!spaceFound)
            {
                basicString[i++] = ' ';
                spaceFound = true;
            }
        }
    }

    // Remove trailing spaces
    if (i <= 1)
        basicString.erase(basicString.begin() + i, basicString.end());
    else
        basicString.erase(basicString.begin() + i - 1, basicString.end());

    return basicString;
}

// will be used to parse the data and store it in a vector
vector<vector<string> > parseData(string input){
    // split input into the commands and, if there are, file paths
    vector<vector<string> > stringParsed;
    vector<string> temp;
    size_t posSpace, posAmper;
    string command;

    do{
        posAmper = input.find('&');
        if(posAmper != string :: npos){
            command = input.substr(0,posAmper);
            input.erase(0, posAmper + 1); // +1 is the size the & takes but program doesn't want the &
        }
        else{
            command = input;
        }
        temp.clear();
        string spacesCheck = command;
        // check for empty string
        if(!spacesCheck.empty()){
            while((posSpace = spacesCheck.find(' ')) != string :: npos){
                string strTemp = spacesCheck.substr(0, posSpace);
                temp.push_back(strTemp);
                // +1 is the size the space takes but program doesn't want the space
                spacesCheck.erase(0, posSpace + 1);
            }
            temp.push_back(spacesCheck);
            // push temp into stringParse
            stringParsed.push_back(temp);
        }
    }while(posAmper != string :: npos);

    // return the vector
    return stringParsed;
    // remove the spaces from input
    //input.erase(remove_if(input.begin(),input.end(),' '),input.end());
}

// will fork the process and execute the command
int forkHere(vector<string> command, vector<string> &path) { //TODO make forHere() good for multiple commands
    unsigned int sizeOfCommand = command.size();
    // get the good path first
    string goodPath = validPath(command, path);

    // if goodPath is -1, no need to fork
    if(goodPath == "-1"){
        write(STDERR_FILENO, error_message, strlen(error_message));
        return 0;
    }
    // fork to create child process
    pid_t ret = fork();
    if(ret < 0){
        // fork failed and there was only one process
        cerr << "fork failed" << endl;
    }
    else if(ret == 0) { // this is the child process
        int goodRedirect = validRedirect(command);
        // goodRedirect is -1, then return after printing error message
        if(goodRedirect == -1){
            write(STDERR_FILENO, error_message, strlen(error_message));
            kill(getpid(),SIGKILL);
        }
        // as the child, we can now create a completely different process that will be independent of the parent
        char **my_args = new char*[sizeOfCommand + 1];
        // first command will be
        //my_args[0] = strdup(goodPath.c_str());
        for(unsigned int i = 0; i < sizeOfCommand; i++){
            my_args[i] = strdup(command[i].c_str());
        }
        // add nullptr as the last element
        my_args[sizeOfCommand] = NULL;
        // call execv for arguments
        execv(goodPath.c_str(), my_args);
        // if program is here, then there is something wrong
        write(STDERR_FILENO, error_message, strlen(error_message));
        kill(getpid(),SIGKILL);
        // no need to delete dynamic memory cause execv already made this process go bye bye
    }
    else{ // this is the parent process
        // when will I use the parent process?
        // wait until ANY child has finished its process
        wait(nullptr); // use waitpid() for waiting for a specific child
        return 0;
    }
    return getpid();
}

// will get the path vector and check for the first valid path. if none are found then return empty string
string validPath(vector<string> command, vector<string> &path) {
    string goodPath;
    //cout << "goodPath is " << goodPath << endl;
    // while loop goes through path vector and matches with first valid access
    unsigned int idx = 0;
    int good;
    while(idx < path.size()){
        goodPath = path[idx] + command[0];
        good = access(goodPath.c_str(), X_OK);
        if(good == 0){
            //goodPath = path[idx] + command[0];
            break;
        }
        else{
            idx++;
        }
    }
    // do one more check to see if any paths were valid
    if(access(goodPath.c_str(), X_OK) != 0){
        // bad access so print error message and return to main
        //cout << "All paths were bad goodPath is " << goodPath << endl;
        // kill child process
        //kill(getpid(),SIGKILL);
        return "-1";
    }
    else{
        return goodPath;
    }
}

// will go through command and check for > and handle redirection
int validRedirect(vector<string> &command){
    string remember; // will be the file program will create, if it happens
    unsigned int howBig = command.size(); // bc command size might change when program erases from vector
    // go through command vector and look for >
    for(unsigned int i = 0; i < howBig; i++){
        if(command[i] == ">"){ // if there is a space after the redirection
            // check that there is something after the > or if there are multiple files after >
            if(i != howBig -2 || i == 0){ //i=0 means that there was no command before > so error
                // there is no file or too many files so error and return
                return -1;
            }
            else{
                remember = command[i+1];
                // so program can remove > and the element after
                command.erase(command.begin()+i,command.begin() + i+2);
                howBig = command.size();
                // to stop the loop
                i = howBig;
            }
        }
        else if(command[i][0] == '>'){ // if there are no spaces after the redirection
            string temp = command[i];
            // get rid of the > sign
            temp.erase(temp.begin());
            remember = temp;
            // so program can remove > string
            command.erase(command.begin()+i);
            howBig = command.size();
            i = howBig; // to stop the loop
        }
        else if(command[i].find('>') != string :: npos){// somewhere in middle of the string is the >
            string temp1 = command[i];
            // create substring that is the string before the > and assign to vector
            string beforeTheSign = temp1.substr(0,temp1.find('>'));
            command[i] = beforeTheSign;
            // have remember be the substring after the >
            remember = temp1.substr(temp1.find('>')+1);
            i = howBig; // to stop the loop
        }
    }
    // check that remember is not empty string
    if(!remember.empty()){
        // open file
        int redirect_fd = open(remember.c_str(),O_CREAT | O_TRUNC | O_WRONLY);
        // check if file was opened successfully
        if(redirect_fd > -1){
            dup2(redirect_fd,STDOUT_FILENO);
            dup2(redirect_fd,STDERR_FILENO);
            // close file
            close(redirect_fd);
        }
    }
    // everything was good so return 0
    return 0;
}
