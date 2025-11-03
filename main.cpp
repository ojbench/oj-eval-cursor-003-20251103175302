#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <sstream>

using namespace std;

struct Submission {
    string problem;
    string status;
    int time;
};

struct ProblemStatus {
    bool solved = false;
    int solveTime = 0;
    int wrongAttempts = 0;
    int frozenSubmissions = 0;
    vector<Submission> submissions;
    vector<Submission> frozenSubs;  // Submissions made during freeze
};

class Team {
public:
    string name;
    map<char, ProblemStatus> problems;
    
    // Cached values for performance
    mutable int cachedSolved = -1;
    mutable int cachedPenalty = -1;
    mutable vector<int> cachedTimes;
    mutable bool cacheValid = false;
    
    Team(string n) : name(n) {}
    
    void invalidateCache() const {
        cacheValid = false;
    }
    
    void updateCache() const {
        if (cacheValid) return;
        
        cachedSolved = 0;
        cachedPenalty = 0;
        cachedTimes.clear();
        
        for (const auto& p : problems) {
            if (p.second.solved) {
                cachedSolved++;
                cachedPenalty += p.second.solveTime + 20 * p.second.wrongAttempts;
                cachedTimes.push_back(p.second.solveTime);
            }
        }
        sort(cachedTimes.rbegin(), cachedTimes.rend());
        cacheValid = true;
    }
    
    int getSolvedCount() const {
        updateCache();
        return cachedSolved;
    }
    
    int getPenaltyTime() const {
        updateCache();
        return cachedPenalty;
    }
    
    const vector<int>& getSolveTimes() const {
        updateCache();
        return cachedTimes;
    }
};

class ICPCSystem {
private:
    map<string, Team*> teams;
    bool started = false;
    bool frozen = false;
    int freezeTime = -1;
    int durationTime = 0;
    int problemCount = 0;
    vector<string> ranking;
    
    bool compareTeams(const string& t1Name, const string& t2Name) {
        Team* team1 = teams[t1Name];
        Team* team2 = teams[t2Name];
        
        int solved1 = team1->getSolvedCount();
        int solved2 = team2->getSolvedCount();
        if (solved1 != solved2) return solved1 > solved2;
        
        int penalty1 = team1->getPenaltyTime();
        int penalty2 = team2->getPenaltyTime();
        if (penalty1 != penalty2) return penalty1 < penalty2;
        
        const vector<int>& times1 = team1->getSolveTimes();
        const vector<int>& times2 = team2->getSolveTimes();
        
        size_t minSize = min(times1.size(), times2.size());
        for (size_t i = 0; i < minSize; i++) {
            if (times1[i] != times2[i]) return times1[i] < times2[i];
        }
        
        return t1Name < t2Name;
    }
    
    void flushScoreboard() {
        ranking.clear();
        for (auto& p : teams) {
            ranking.push_back(p.first);
        }
        sort(ranking.begin(), ranking.end(), [this](const string& a, const string& b) {
            return compareTeams(a, b);
        });
    }
    
    string getProblemDisplay(const ProblemStatus& ps, bool isFrozen) {
        if (isFrozen) {
            if (ps.wrongAttempts == 0) {
                return "0/" + to_string(ps.frozenSubmissions);
            } else {
                return "-" + to_string(ps.wrongAttempts) + "/" + to_string(ps.frozenSubmissions);
            }
        } else if (ps.solved) {
            if (ps.wrongAttempts == 0) return "+";
            return "+" + to_string(ps.wrongAttempts);
        } else {
            if (ps.wrongAttempts == 0) return ".";
            return "-" + to_string(ps.wrongAttempts);
        }
    }
    
    void printScoreboard() {
        for (const string& teamName : ranking) {
            Team* team = teams[teamName];
            int rank = 0;
            for (size_t i = 0; i < ranking.size(); i++) {
                if (ranking[i] == teamName) {
                    rank = i + 1;
                    break;
                }
            }
            
            cout << teamName << " " << rank << " " 
                 << team->getSolvedCount() << " " 
                 << team->getPenaltyTime();
            
            for (int i = 0; i < problemCount; i++) {
                char prob = 'A' + i;
                const ProblemStatus& ps = team->problems[prob];
                bool isFrozen = frozen && !ps.solved && ps.frozenSubmissions > 0;
                cout << " " << getProblemDisplay(ps, isFrozen);
            }
            cout << "\n";
        }
    }
    
public:
    void addTeam(const string& name) {
        if (started) {
            cout << "[Error]Add failed: competition has started.\n";
            return;
        }
        if (teams.find(name) != teams.end()) {
            cout << "[Error]Add failed: duplicated team name.\n";
            return;
        }
        teams[name] = new Team(name);
        ranking.push_back(name);
        sort(ranking.begin(), ranking.end());
        cout << "[Info]Add successfully.\n";
    }
    
    void startCompetition(int duration, int problems) {
        if (started) {
            cout << "[Error]Start failed: competition has started.\n";
            return;
        }
        started = true;
        durationTime = duration;
        problemCount = problems;
        
        // Initialize problem structures for all teams
        for (auto& p : teams) {
            for (int i = 0; i < problemCount; i++) {
                char prob = 'A' + i;
                p.second->problems[prob] = ProblemStatus();
            }
        }
        
        cout << "[Info]Competition starts.\n";
    }
    
    void submit(const string& problem, const string& teamName, const string& status, int time) {
        Team* team = teams[teamName];
        char prob = problem[0];
        ProblemStatus& ps = team->problems[prob];
        
        Submission sub = {problem, status, time};
        ps.submissions.push_back(sub);
        
        if (frozen && !ps.solved) {
            ps.frozenSubmissions++;
            ps.frozenSubs.push_back(sub);
        } else if (!ps.solved) {
            if (status == "Accepted") {
                ps.solved = true;
                ps.solveTime = time;
                team->invalidateCache();
            } else {
                ps.wrongAttempts++;
                team->invalidateCache();
            }
        }
    }
    
    void flush() {
        flushScoreboard();
        cout << "[Info]Flush scoreboard.\n";
    }
    
    void freeze() {
        if (frozen) {
            cout << "[Error]Freeze failed: scoreboard has been frozen.\n";
            return;
        }
        frozen = true;
        cout << "[Info]Freeze scoreboard.\n";
    }
    
    void scroll() {
        if (!frozen) {
            cout << "[Error]Scroll failed: scoreboard has not been frozen.\n";
            return;
        }
        
        cout << "[Info]Scroll scoreboard.\n";
        
        // Flush first
        flushScoreboard();
        printScoreboard();
        
        // Unfreeze process
        while (true) {
            bool found = false;
            string targetTeam;
            char targetProblem = 'Z' + 1;
            
            // Find lowest-ranked team with frozen problems
            for (int i = ranking.size() - 1; i >= 0; i--) {
                Team* team = teams[ranking[i]];
                char smallestFrozen = 'Z' + 1;
                
                for (int j = 0; j < problemCount; j++) {
                    char prob = 'A' + j;
                    ProblemStatus& ps = team->problems[prob];
                    if (ps.frozenSubmissions > 0) {
                        smallestFrozen = min(smallestFrozen, prob);
                    }
                }
                
                if (smallestFrozen <= 'Z') {
                    targetTeam = ranking[i];
                    targetProblem = smallestFrozen;
                    found = true;
                    break;
                }
            }
            
            if (!found) break;
            
            // Unfreeze the problem
            Team* team = teams[targetTeam];
            ProblemStatus& ps = team->problems[targetProblem];
            
            int oldRank = 0;
            for (size_t i = 0; i < ranking.size(); i++) {
                if (ranking[i] == targetTeam) {
                    oldRank = i;
                    break;
                }
            }
            
            // Process frozen submissions for this problem
            bool changed = false;
            for (const auto& sub : ps.frozenSubs) {
                if (!ps.solved) {
                    if (sub.status == "Accepted") {
                        ps.solved = true;
                        ps.solveTime = sub.time;
                        changed = true;
                    } else {
                        ps.wrongAttempts++;
                    }
                }
            }
            ps.frozenSubmissions = 0;
            ps.frozenSubs.clear();
            
            if (!changed) continue;  // No ranking change if not accepted
            
            team->invalidateCache();
            
            // Find new position by comparing with teams above
            int newRank = oldRank;
            while (newRank > 0 && compareTeams(targetTeam, ranking[newRank - 1])) {
                newRank--;
            }
            
            if (newRank < oldRank) {
                // Remove from old position and insert at new position
                ranking.erase(ranking.begin() + oldRank);
                ranking.insert(ranking.begin() + newRank, targetTeam);
                
                cout << targetTeam << " " << ranking[newRank + 1] << " " 
                     << team->getSolvedCount() << " " << team->getPenaltyTime() << "\n";
            }
        }
        
        frozen = false;
        printScoreboard();
    }
    
    void queryRanking(const string& teamName) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query ranking failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query ranking.\n";
        if (frozen) {
            cout << "[Warning]Scoreboard is frozen. The ranking may be inaccurate until it were scrolled.\n";
        }
        
        int rank = 0;
        for (size_t i = 0; i < ranking.size(); i++) {
            if (ranking[i] == teamName) {
                rank = i + 1;
                break;
            }
        }
        
        cout << teamName << " NOW AT RANKING " << rank << "\n";
    }
    
    void querySubmission(const string& teamName, const string& problem, const string& status) {
        if (teams.find(teamName) == teams.end()) {
            cout << "[Error]Query submission failed: cannot find the team.\n";
            return;
        }
        
        cout << "[Info]Complete query submission.\n";
        
        Team* team = teams[teamName];
        Submission* lastMatch = nullptr;
        
        for (auto& p : team->problems) {
            for (auto& sub : p.second.submissions) {
                bool problemMatch = (problem == "ALL" || sub.problem == problem);
                bool statusMatch = (status == "ALL" || sub.status == status);
                
                if (problemMatch && statusMatch) {
                    if (!lastMatch || sub.time > lastMatch->time) {
                        lastMatch = &sub;
                    }
                }
            }
        }
        
        if (!lastMatch) {
            cout << "Cannot find any submission.\n";
        } else {
            cout << teamName << " " << lastMatch->problem << " " 
                 << lastMatch->status << " " << lastMatch->time << "\n";
        }
    }
    
    void end() {
        cout << "[Info]Competition ends.\n";
    }
    
    ~ICPCSystem() {
        for (auto& p : teams) {
            delete p.second;
        }
    }
};

int main() {
    ICPCSystem system;
    string line;
    
    while (getline(cin, line)) {
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        
        if (cmd == "ADDTEAM") {
            string teamName;
            iss >> teamName;
            system.addTeam(teamName);
        }
        else if (cmd == "START") {
            string duration, problem;
            int durationTime, problemCount;
            iss >> duration >> durationTime >> problem >> problemCount;
            system.startCompetition(durationTime, problemCount);
        }
        else if (cmd == "SUBMIT") {
            string problemName, by, teamName, with, status, at;
            int time;
            iss >> problemName >> by >> teamName >> with >> status >> at >> time;
            system.submit(problemName, teamName, status, time);
        }
        else if (cmd == "FLUSH") {
            system.flush();
        }
        else if (cmd == "FREEZE") {
            system.freeze();
        }
        else if (cmd == "SCROLL") {
            system.scroll();
        }
        else if (cmd == "QUERY_RANKING") {
            string teamName;
            iss >> teamName;
            system.queryRanking(teamName);
        }
        else if (cmd == "QUERY_SUBMISSION") {
            string teamName, where, rest;
            iss >> teamName >> where;
            getline(iss, rest);
            
            size_t problemPos = rest.find("PROBLEM=");
            size_t statusPos = rest.find("STATUS=");
            size_t andPos = rest.find(" AND ");
            
            string problem = rest.substr(problemPos + 8, andPos - (problemPos + 8));
            string status = rest.substr(statusPos + 7);
            
            system.querySubmission(teamName, problem, status);
        }
        else if (cmd == "END") {
            system.end();
            break;
        }
    }
    
    return 0;
}
