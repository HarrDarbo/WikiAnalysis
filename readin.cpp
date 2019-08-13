#include <iostream>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <vector>
#include <algorithm>
#include <string.h>
#include <bits/stdc++.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unordered_map>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/wait.h>

typedef std::unordered_map<std::string, int> hlist; //structure for hscores
typedef std::unordered_map<std::string, bool> inlist; //structure for finished hscores
typedef std::unordered_map<std::string, std::string> relist; //structure for redirects

//global because its small and convenient
int writecount = 0;
std::string type;
int deep = 0;

//computation methods
void readarticle(std::string name, std::string &article, std::ofstream &namelist, inlist &dirs); //make text files from XML
void readarticleL(std::string name, std::string &article, std::ofstream &namelist, inlist &dirs); //make text, but feed to linkArticlesR
void linkarticles(std::string filename, std::string dirname, std::string directory); //make web files from existing text files
void linkarticlesR(std::string filename, std::string dirname, std::string &article); //makes web files from info from readarticleL
int Hconnect(hlist &ascore, inlist &in, std::string name, std::string path, inlist &articles); //connect all webs, returns scores
void HconnectNew(hlist &ascore, std::vector<std::string> &names, relist &redirs);

//methods for parsing or searching
void removeChars(std::string &str, char* charsToRemove); //remove all instances of characters
bool containsChars(std::string &str, const char* charsToRemove); //does this contain these characters
bool prohibitedName(std::string str); //windows prohibited file names
void redirectadd(std::string name, std::string &article, std::ofstream &wredirect); //add a redirect to redirect file
std::string finddir(std::string &name); //find the directory for the given file name
bool badAType(std::string &name); //finds non-article pages

int main(int argc, char* argv[]){
    /*struct rlimit rl;
    getrlimit(RLIMIT_NPROC,&rl);
    printf("org limit is %lld\n", (long long int)rl.rlim_cur);
    rl.rlim_cur = -1;
    rl.rlim_max = -1;
    setrlimit(RLIMIT_NPROC,&rl);
    //sleep(1);
    getrlimit(RLIMIT_NPROC,&rl);
    printf("new limit is %lld\n", (long long int)rl.rlim_cur);*/
    std::ifstream read("articles.xml"); //XML file to be read
    if (!read.good()) { //testing for valid file
        std::cerr << "Can't open " << argv[1] << " to read.\n";
        exit(1);
    }
    std::string word; //base word to be read in
    std::string nextword; //for word concatenation
    int wordsread = 0;
    int next = -1; //whether or not to continue from manual progression
    int option = -1;

    int skip = 0; //number of initial skip lines
    for(int x = 0;x<skip;x++) read>>word; //skipping X words, testing purposes only

    std::cout<<"What would you like to do?\n1:Manual Walking\n2:Create info file system (Long Run Time)\n3:Analyze a dataset (Very Long Run Time)\n4:Validate File System (Sorta Long Run Time)\n";
    std::cin>>option;

    if(option == 1){ //MANUAL WALKING
        //MANUAL WALKING: Testing purposes only really, but a default option for now.
        std::ifstream read("OtherData/redirects.txt");
        while(next != 0){
            std::cout<<"Read (# of Words): "; //Words may not directly correlate to "words", but it's too insignificant to care about
            std::cin>>next; //next reads amount of words
            for(int x = 0;x<next;x++){
                read>>word; //word is current word
                wordsread++; //counter
                if(word.find("<")!=std::string::npos){ //taking out XML formatting
                    while(word.find(">")==std::string::npos){ //finding everything contained in two triangle brackets
                        read>>nextword;
                        wordsread++;
                        word+=" ";
                        word+=nextword;
                    }
                    std::cout<<word<<std::endl;
                    x--; //XML formatting won't count towards word count
                }else{
                    std::cout<<word<<" ";//just printing it out; once again, only for testing purposes
                }
            }
            std::cout<<std::endl;
        }

    }else if(option == 2){ //MAKING ARTICLE DATABASES
        /**CREATING ARTICLE DATABASE: Main Purpose, can make raw articles or webs**/

        std::cout<<"Would you like to create (T) Pure text files or (W) Web link files?"<<std::endl;
        std::cin>>type;
        std::ofstream namelist;
        if (mkdir("OtherData", 0777) == -1){
            std::cerr << "Error :  " << strerror(errno) << std::endl;  //Initializing Directory
        }else std::cout << "OtherData Directory created\n";

        std::ofstream wredirect("OtherData/redirects.txt");

        if(type == "T"){
            type = "Articles";
            if (mkdir("Articles", 0777) == -1){
                std::cerr << "Error :  " << strerror(errno) << std::endl;  //Initializing Directory
            }else std::cout << "Article Directory created\n";
            std::ofstream namelist("OtherData/!!!articlenames.txt"); //a big'ol list of article names, just incase.
            namelist<<"Article Names:\n";
        }else if(type == "W"){
            type = "Webs";
            if (mkdir("Webs", 0777) == -1) std::cerr << "Error:  " << strerror(errno) << std::endl; //Initializing Directory
            else std::cout << "Web Directory created\n";
            std::ofstream namelist("OtherData/!!!articlenames.txt"); //a big'ol list of article names, just incase.
            namelist<<"Article Names:\n";
        }
        bool nart = false; //not article
        bool yred = false; //if redirect
        std::cout<<"Enter a negative number for entire database scan, or enter 0 to stop scanning."<<std::endl; //negative number is arbitrary, just roll with it
        while(next != 0){
            std::cout<<"Read (# of Articles): ";
            std::cin>>next;
            inlist dirs;
            std::string name;
            int red = 0;//counter for set reading account
            while(read>>word){
                if(next<0){
                }else if(red>=next) break;

                if(word.find("<title>")!=std::string::npos){ //starting article where we care; in the title

                    if(word.find("<redirect")!=std::string::npos) nart = true; //checking for invalid article type redirect (different wa of finding than other unaccepted article types)
                    while(word.find("</text>")==std::string::npos){ //until end of article
                        read>>nextword;
                        if(nextword.find("<redirect")!=std::string::npos){
                          yred = true; //is redirect to be saved
                          nart = true; //but its not an article
                        }
                        word+=" ";
                        word+=nextword;
                    }
                    name = word.substr(word.find("<title>")+7,word.find("</title>")-7); //extracting title
                    transform(name.begin(), name.end(), name.begin(), ::tolower); //to lowercase
                    if(!nart) red++; //article count for arbitrary amount
                    nart = badAType(name);
                    if(!yred && !nart) writecount++;
                    pid_t pid = fork();
                    if(pid == -1) return 0;
                    if(pid > 0){
                      int status;
                      waitpid(-1,&status,WNOHANG);
                    }
                     if(pid == 0){
                      //std::cout << "#STR# " << red << std::endl;
                      if(yred){
                        redirectadd(name, word, wredirect);
                      }else if(!nart){
                        if(type == "Webs") readarticleL(name,word,namelist,dirs); //read article is where each individual article is parsed
                        else if(type == "Articles") readarticle(name,word,namelist,dirs); //read article is where each individual article is parsed
                      }
                      //std::cout << "|END| " << red << std::endl;
                      return 0;
                    }

                    nart = false;
                    yred = false;
    }}}
        std::cout<<std::endl;
    }else if(option == 3){ //MAKING HLIST
        /**ANALYSIS BASED ON PREVIOUSLY GENERATED DATABASES**/
        ///at the moment its only text to webs
        std::cout<<"Would you like to\n(W) Create Web link files (based on existing text files)\n(H) Calculate the Hitler Score? (Faster, Less Accurate)\n(HN) Calculate the Hitler Score? (Slower, More Accurate)"<<std::endl;
        std::cin>>type;
        if(type == "W"){
            if (mkdir("Web", 0777) == -1) std::cerr << "Error :  " << strerror(errno) << std::endl;
            else std::cout << "Directory created\n";
            std::string direc = "Articles";
            DIR* dirp = opendir(direc.c_str());
            struct dirent * dp;
            while ((dp = readdir(dirp)) != NULL) {
                std::string loc = dp->d_name;
                std::cout<<loc<<std::endl;
                loc = "Articles/" + loc;
                if(loc.find(".txt")==std::string::npos){
                    DIR* dirpdeep = opendir(loc.c_str());
                    struct dirent * dpdeep;
                    while ((dpdeep = readdir(dirpdeep)) != NULL) {
                        std::string filename = dpdeep->d_name;
                        std::cout<<"     "<<filename<<" ";
                        ///CREATE WEB FILES HERE
                        linkarticles(filename, loc, dp->d_name);
                    }
                    closedir(dirpdeep);
            }}
            closedir(dirp);
        }else if (type == "H"){
            type = "Webs";
            std::ifstream file("OtherData/!!!articlenames.txt");
            std::string name;
            inlist names;
            while(file >> name) names[name] = true;
            hlist articlescore;
            inlist in;
            inlist dirs;
            articlescore["adolf hitler"] = 0;
            std::string direc = "./Webs";
            DIR* dirp = opendir(direc.c_str());
            struct dirent * dp;
            readdir(dirp);
            readdir(dirp);
            int temp = 0;
            while ((dp = readdir(dirp)) != NULL) {
                std::string loc = dp->d_name;
                std::cout<<"1:"<<loc<<std::endl;
                loc = "Webs/" + loc;
                DIR* dirpdeep = opendir(loc.c_str());
                struct dirent * dpdeep;
                while ((dpdeep = readdir(dirpdeep)) != NULL) {
                    std::string filename = dpdeep->d_name;
                    std::cout<<"2:"<<filename<<std::endl;
                    articlescore[filename] = -1;
                    articlescore[filename] = Hconnect(articlescore, in, filename, loc+"/"+filename, names);
                    temp++;
                    if(temp >= 3) exit(0);
                    sleep(1);
                    std::cout << filename << " distance: " << articlescore[filename] << std::endl;
                    in.clear();
                }
                closedir(dirpdeep);
            }
            closedir(dirp);
            std::ofstream whscore("OtherData/hscores(unorganized).txt");
            for(hlist::iterator itr = articlescore.begin();itr != articlescore.end();itr++){
              whscore << itr->first << "::" << itr->second << std::endl;
            }
        }else if(type == "HN"){
          hlist articlescore;
          int testCount = 0;
          std::vector<std::string> names;
          std::string direc = "./Webs";
          DIR* dirp = opendir(direc.c_str());
          struct dirent * dp;
          readdir(dirp);
          readdir(dirp);
          while ((dp = readdir(dirp)) != NULL) {
              std::string loc = dp->d_name;
              std::cout<<"Scanning in "<<loc<< "..."<<std::endl;
              loc = "Webs/" + loc;
              DIR* dirpdeep = opendir(loc.c_str());
              struct dirent * dpdeep;
              while ((dpdeep = readdir(dirpdeep)) != NULL) {
                  std::string filename = dpdeep->d_name;
                  if(filename != "." || filename != ".."){
                    std::string nrmlname = filename.substr(0,filename.find(".txt"));
                    names.push_back(nrmlname);
                    //std::cout<<"2:"<<filename<<std::endl;
                    testCount++;
                    articlescore[nrmlname] = -1;
                    if(testCount > 300000) goto skip;
                  }
              }
              closedir(dirpdeep);
          }
          closedir(dirp);
          skip:;
          articlescore["adolf hitler"] = 0;
          std::cout<<"Scanning in redirects..."<<std::endl;
          relist redirects;
          std::ifstream redir("OtherData/redirects.txt");
          std::string rline,rstart,rend;
          while(getline(redir,rline)){
            rstart = rline.substr(0,rline.find("::"));
            rend = rline.substr(rline.find("::")+2);
            redirects[rstart] = rend;
          }
          std::cout<<"Starting Analysis!"<<std::endl;
          type = "Webs";
          HconnectNew(articlescore, names, redirects);


        }
    }else if(option == 4){ //VALIDATE FILE SYSTEM
      std::cout << "Beginning Validation...\n";
      int fileCount = 0,fileCountInit;
      std::string direc = "Webs";
      DIR* dirp = opendir(direc.c_str());
      struct dirent * dp;
      while ((dp = readdir(dirp)) != NULL) {
        //std::cout << "Opening Folder " << dp->d_name << std::endl;
        fileCountInit = fileCount;
        std::string loc = dp->d_name;
        loc = "Webs/" + loc;
        DIR* dirpdeep = opendir(loc.c_str());
        struct dirent * dpdeep;
        while ((dpdeep = readdir(dirpdeep)) != NULL){
          fileCount++;
          //std::cout << "File " << dpdeep->d_name << " found." << std::endl;
        }
        closedir(dirpdeep);
        std::cout << "File Count in " << loc << ": " << fileCount-fileCountInit << " (" << fileCount << " overall)" << std::endl;
      }
      closedir(dirp);
      std::cout << "Total File Count of Directory " << direc << ": " << fileCount;
    }else{
        std::cout<<"Unknown option selected. Program Quitting.";
    }
    std::cout<<std::endl;
}

void readarticle(std::string name, std::string &article,std::ofstream &namelist, inlist &dirs){
    std::string dirname = finddir(name);
    if(!dirs[dirname]){
      if (mkdir(dirname.c_str(), 0777) != -1) std::cout<<"New Directory: "<<dirname<<std::endl; //making new directory if new index
    }


    if(writecount % 10000 == 0){ //every 10k articles, update the user on profress and confirming its still running
        float percent = (double)writecount/(double)5861045; //This number is not arbitrary; it was the total article count of wikipedia as of May 2019.
        //if this number goes over, it means (A) the current XML being scanned is newer than when the program was created or (B) non-articles arent being weeded out
        percent*=(double)100;
        std::cout<<writecount<<" articles read, or "<<percent<<"% done. Latest article: "<<name<<std::endl;
    }
    namelist<<name<<"||"; //simple arbitrary separating in articlenames txt file
    std::string filename = dirname + "/" +  name  + ".txt"; //new txt file name
    std::ofstream articletxt(filename); //the new file
    articletxt<<name<<std::endl<<article<<std::endl; //give the file the name & full article
    articletxt.close(); //close it, then return
}
void readarticleL(std::string name, std::string &article,std::ofstream &namelist, inlist &dirs){
    std::string dirname = finddir(name);
    if(!dirs[dirname]){
      if (mkdir(dirname.c_str(), 0777) != -1) std::cout << "New Directory: "<<dirname<<std::endl; //making new directory if new index
      dirs[dirname] = true;
    }

    if(writecount % 10000 == 0){
        float percent = (double)writecount/(double)5861045;
        percent*=(double)100;
        std::cout<<writecount<<" articles read & analyzed, or "<<percent<<"% done. Next article: "<<name<<std::endl;
    }
    ///Everything in this function is functionally the same as the previous one, except for this ending part
    std::string filename = dirname + "/" +  name  + ".txt"; //new file
    linkarticlesR(filename,dirname,article); ///pass the info to another function to find web
}

int Hconnect(hlist &ascore, inlist &in, std::string name, std::string path, inlist &articles){
    int val = ascore[name];
    in[name] = true;
    inlist::iterator initr;
    int top = -3;
    if(val == -1){ //if not in the hash map already
        top = 9999999;
        std::cout << path << std::endl;
        std::ifstream file(path); //read in contents from parsed file
        if(!file.good()) return -2;
        std::vector<std::string> links; //all the links in one particular file
        std::string link;
        while(file>>link){
          //std::cout << link << std::endl;
            if(link == "adolf hitler"){
                ascore[name] = 1;
                return 1;
            }
            links.push_back(link);
        } //by here we have a valid vector of links
        int distance;
        for(unsigned int x = 0;x<links.size();x++){ //for all links in web file
            initr = in.find(links[x]); //checking if score already found
            if(initr == in.end()){
              deep++;
              std::cout << deep << ": ITERATE DEEPER" << std::endl;

              std::string dir = finddir(links[x]) + "/" + links[x] + ".txt";
              //sleep(1);
              ascore[links[x]] = -1;
              distance = Hconnect(ascore,in,links[x],dir,articles)+1; //if not found, recur and find that link's hscore
              if(distance == 2) x = links.size()+2;

              std::cout << deep << ": ITERATE END" << std::endl;
              deep--;
            }else distance = ascore[links[x]] +1; //if found, use and add 1
            if(distance < top && distance >= 0) top = distance; //if smallest distance, use
        }
        std::cout << "  " << name << " distance: " << distance << std::endl;
    }else{ //file has aleady been calculated
        return ascore[name];
    }
    return top;
}

void HconnectNew(hlist &ascore, std::vector<std::string> &names, relist &redirs){
  std::unordered_set<std::string> list;
  std::unordered_set<std::string> listtemp;
  list.insert("adolf hitler"); //SEED ARTICLE

  std::ofstream whscore("OtherData/hscores(organized).txt");

  int score = 1;
  startJump:;
  int counter = 0;
  std::cout<<"Finding Distances of " << score << std::endl;
  bool remain = false;
  for(unsigned int x = 0;x<names.size();x++){
    if(x % 10000 == 0) std::cout << "(Dist " << score << ") Name index " << x << std::endl;
    std::string filename = names[x];
    if(ascore[filename] == -1){
      remain = true;
      std::string path = finddir(filename) + "/" + filename + ".txt";
      //std::cout << filename << " is found in " << path << std::endl;
      std::ifstream file(path); //read in contents from parsed file
      if(file.good()){
        std::string link;
        while(getline(file,link)){
          //if(link == "adolf hitler") std::cout << *list.find(link) << std::endl;
          if(list.find(link) == list.end() && redirs.find(link) != redirs.end()){
            if(link == "adolf hitler") std::cout << "redir at " << link << std::endl;
            link = redirs.find(link)->second;
          }
          if(list.find(link) != list.end()){
            //if(score == 1) std::cout << "Found one! Direct link to " << link << std::endl;
            ascore[filename] = score;
            listtemp.insert(filename);
            counter++;
            if(counter%100 == 0) std::cout<< counter << " files of distance " << score << " found so far. " << std::endl;
            whscore << filename << "::" << score << std::endl;
            names.erase(names.begin()+x);
            x--;
            goto nextFile;
          }

        }
      }
    }
    nextFile:;
  }
  std::cout<< counter << " articles found with distance of " << score << "." << std::endl;
  std::cout << names.size() << " articles remain untouched. " << std::endl;
  score++;
  list = listtemp;
  listtemp.clear();
  if(counter == 0) goto End;
  if(remain) goto startJump;
  End:;
  for(hlist::iterator itr = ascore.begin();itr != ascore.end();itr++){
    if(itr->second == -1) whscore << itr->first << "::" << itr->second << std::endl;
  }
}

void linkarticles(std::string filename, std::string dirname, std::string directory){ //this one is for taking a text file system and turning it into webs
    std::string aname = dirname + "/" + filename; //location in articles
    std::string wname = "Webs/" + directory + "/" + filename; //new location in webs
    std::ofstream warticle(wname);
    std::ifstream aarticle(aname);

    std::string nextword, words = "";
    std::string link;
    while(aarticle>>nextword){ //take article out of articles file
        words+=nextword;
    }
    warticle<<filename<<std::endl;
    while(words.find("[[")!=std::string::npos){ //find the links in XML format
        link = words.substr(words.find("[[")+2,words.find("]]")-2);
        words = words.substr(words.find("]]"));
        transform(link.begin(), link.end(), link.begin(), ::tolower);
        warticle<<link<<std::endl;
    }
    warticle.close();
    if (std::remove(aname.c_str( )) !=0)std::cout<<"Remove operation failed"<<std:: endl; //remove the original file in articles to save space.
    else std::cout<<aname<<" has been removed."<<std::endl;
}
void linkarticlesR(std::string filename, std::string dirname, std::string &article){ //this one is used by readarticleL, directly makes webs
    std::ofstream warticle(filename); //the new file
    std::string link;
    while(article.find("[[")!=std::string::npos){ //links in XML are denoted by [[...]], we're finding them
        unsigned int tempL = article.find("[[")+2;
        unsigned int tempR = article.find("]]");
        unsigned int ref;
        if((ref = article.find("<ref>")) != std::string::npos && ref < tempL){
          ref = article.find("</ref>");
          article = article.substr(ref+6);
        }else if(tempR < tempL-2){ //if end of file markings
            article = article.substr(tempR+2); //cut article to past first link
        }else if (article.substr(tempL, 5).find("File:")==std::string::npos){ //if not beginning of file
            link = article.substr(tempL,tempR-tempL); //extract link
            article = article.substr(tempR+2); //cut article to past first link
            transform(link.begin(), link.end(), link.begin(), ::tolower); //to lowercase
            if(link.find("|")!=std::string::npos) link = link.substr(0,link.find("|")); //how link are parsed in XML, left of | is actual article name
            if(link.find("#")!=std::string::npos) link = link.substr(0,link.find("#")); //how link are parsed in XML, left of | is actual article name
            if(!badAType(link)) warticle<<link<<std::endl; //add link, if an article
        }else{
            article = article.substr(tempL); //accomodating for parsing from files
        }
    }
    warticle.close(); //close and return
}

void redirectadd(std::string name, std::string &article, std::ofstream &wredirect){
  std::string linkedart = article.substr(article.find("<redirect")+17); //extracting title
  linkedart = linkedart.substr(0, linkedart.find("/>")-2);
  wredirect << name << "::" << linkedart << std::endl;
}

std::string finddir(std::string &name){
    std::string dirname;
    std::string chars = "/\\:*?<>|+()@-!$\"";
    ///Creating file name; taking out prohibited characters and names for windows file system
    if(name.length()<2) dirname = name.substr(0,1); //length of file; first two characters at most
    else dirname = name.substr(0,2);
    if(containsChars(dirname,chars.c_str())) dirname = "etc"; //prohibited file characters
    //while(name.find(".")!=std::string::npos) name.replace(name.find("."),1,"(dot)"); //. isnt (totally) allowed, but its very common and i dont want to filter it into etc
    while(dirname.find(".")!=std::string::npos) dirname.replace(dirname.find("."),1,"(dot)");
    if(prohibitedName(name)) name += "(ws)"; //the hard-coded non-allowed windows names
    if(dirname.substr(1,2) == " ") dirname = dirname.substr(0,1) + "(sng)";
    dirname = type + "/" + dirname;
    return dirname;
}
void removeChars(std::string &str, char* charsToRemove ) {
    for (unsigned int i = 0; i < strlen(charsToRemove); ++i ) str.erase(remove(str.begin(), str.end(), charsToRemove[i]), str.end() );
}
bool containsChars(std::string &str, const char* charsToRemove ) {
    for ( unsigned int i = 0; i < strlen(charsToRemove); ++i ) {
        if(str.find(charsToRemove[i])!=std::string::npos) return true;
    }
    return false;
}
bool prohibitedName(std::string str){
    if(str == "con"||str == "prn"||str == "aux"||str == "nul") return true;
    if(str.length() == 4 && (str.substr(0,3) == "COM" || str.substr(0,3) == "LPT") && isdigit(str[3])) return true;
    return false;
}
bool badAType(std::string &name){
  std::string articletypes[8] = {"category:","template:","(disambiguation)","mediawiki:","portal:","book:","timedtext:","module:"}; //unwanted filetypes
  for(int x = 0;x<8;x++){ //checking all other invalid article types
      if(name.find(articletypes[x])!=std::string::npos) return true;
  }
  return false;
}
