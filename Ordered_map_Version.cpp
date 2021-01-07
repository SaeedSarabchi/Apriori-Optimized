//
//  main.cpp
//  Apriori
//
//  Created by sasa on 2/12/17.
//  Copyright © 2017 sasa. All rights reserved.
//

#include <iostream>
#include<fstream>
#include <vector>
#include <string>
#include <sstream>
#include <iterator>
#include <unordered_map>
#include <map>
#include <ctime>
#include <algorithm>
#include <utility>
#include <iomanip>

using namespace std;

//Considering txt file as Database
std::ifstream DB;

//candidates generated from Lk
void GetCandidates(vector<vector<long> >& TempItemSet, vector<vector<long> > KSize_Freq_ItmSet, std::map<vector<long>,bool> KSize_Freq_HashMap, int k);

//increment the count of all candidates in Ck+1 that are contained in transaction
void IncrementHashMapCounter(std::map<vector<long>,long>& HashMap, std::map<vector<long>,bool> CandidateItem_HashMap,  const vector<vector<long> >& CandidateItemSet, int k, unordered_map<long,bool>& UnwantedTransactions);

//To Calculate all of the Subsets of Items with size k
void SetSubsetsOfSizeK(vector<vector<long> >&  TransSubsets,vector<long> Item, int k, int idx, vector<long> current);
//void SetSubsetsOfSizeK(vector<vector<long> >&  TransSubsets,vector<long> Item, int k);

//Generating All of the Strong Association Rules
void Association_Rule_Gen(vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>> & Assoc_Rules, const std::map<vector<long>, long>& FreqItemSet);

//For Outputting all Strong Rules in Output
void OutputStrongRules(const vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>>& Strong_Assoc_Rules);

//Generating All of The No-Empty Subsets of a Set
void SetSubsets(vector<vector<long> >&  TransSubsets,vector<long> Item);

//For outputting all Freq ITemSet with their Support
void OutputFrequentItemSet(const std::map<vector<long>, long>&  FreqItemSet);

//Outputting number of frequent itemsets of different sizes and the number of strong rules
void OutputResultCount(const std::map<vector<long>, long>&  FreqItemSet, const vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>>& Strong_Assoc_Rules, double elapsed_secs);

//Calculating Choose Itemsize from k
unsigned long SubsetCount(unsigned long Itemsize,unsigned long k);

//General Variable Declaration Start
string InputPath; //paht for transaction data
float Min_Support;
float Min_Confidence;
char Action; // either 'r','f' or 'a'
long Num_of_TotalTrans;

string tempStrBuffer;


////////////////////////////////////////////////////
//////////////********BEGIN*********////////////////
////////////////////////////////////////////////////



int main(int argc, const char * argv[]) {
    
    //Starting the Clock for Execution Time
    clock_t begin = clock();
    
    std::map<vector<long>, long> FreqItemSet;
    
    //For Storing Freqent Item Sets of Size K
    vector<vector<long> > KSize_Freq_ItmSet;
    
    //Input Gathering Start
    try {
        InputPath = argv[1]; //location of the input file
        Min_Support = atof(argv[2]); //minimum support between 0 and 1.
        Min_Confidence =atof(argv[3]); //minimum conf between 0 and 1
        
        if (argc==5)
        {
            //Throw execption if the 4th input was wrong
            Action = argv[4][0];
            if(Action!='r' && Action!='f' && Action!='a')
            {
                cout<<"The 4th Parameter Should be Either r, f or a"<<endl;
                return 3;
            }
        }
        if (argc>5)
        {
            cout<<"The Number of Parameters are wrong"<<endl;
            return 3;
        }
    } catch (exception& e) {
        cout<< "An Exception Occured in Input Variables:"<<endl;
        cout << e.what() << endl;
        return 2;
    }
    //Input Gathering End
    
    //Reading Input File
    DB.open(InputPath);
    
    //Considering each line of txt file as a transaction
    string Transaction;
    
    //HashMap input is a set of long, and its counter is of type long
    std::map<vector<long>, long> HashMap; //To Fast Access the Count of Frequent Items
    
    //Reading the file for the first time:
    if (DB.is_open())
    {
        
        vector<long> Item; //Considering each split as an Item
        while (getline(DB, Transaction))
        {
            Num_of_TotalTrans++;
            
            //Splitting the Transaction by Space and Populating 1st Round HashMap Start
            //[Idea from cplusplus.com]
            istringstream buf(Transaction);
            istream_iterator<string> beg(buf), end;
            vector<string> tokens(beg, end);
            
            for(int tokenIndex=0; tokenIndex<tokens.size(); tokenIndex++)
            {
                Item.push_back(stol(tokens[tokenIndex]));
                
                //Check if each Item is in the HashMap or not
                std::map<vector<long>, long>::const_iterator SearchKey = HashMap.find (Item);
                if ( SearchKey == HashMap.end() )
                {
                    //If the Item Does not exist in HashMap, Then Insert it
                    HashMap.insert({Item,1});
                    KSize_Freq_ItmSet.push_back(Item);
                }
                else
                {
                    //If the Item exists in HashMap, Then Add one to its Counter
                    HashMap[Item] = (SearchKey->second)+1;
                }
                Item.erase(Item.begin());
            }
            
            //Split & 1st Round Hashmap Populating End
        }
        Num_of_TotalTrans--;
        
        //Assigning 1st FreqItemSet
        for (auto &h : HashMap)
        {
            if(h.second >= Min_Support*Num_of_TotalTrans)
                FreqItemSet.insert(h);
        }
    }
    
    //The algorithm for finding frequent itemsets Start:
    
    //Candidate ItemSet for pruning the search space
    vector<vector<long> > CandidateItemSet;
    
    //For Fast Accessing The Elements of KSize_Freq_ItmSet
    std::map<vector<long>, bool> KSize_Freq_HashMap;
    for (auto const& set : KSize_Freq_ItmSet)
    {
        KSize_Freq_HashMap.insert({set,true});
    }
    
    //For Fast Accessing The Elements of Candidate_ItemSet
    std::map<vector<long>, bool> Candidate_HashMap;
    
    //Used for execution time testings
    clock_t end ;
    double elapsed_secs;
    
    //The set of Line Numbers Which Do not Contribute to any Freq Set
    unordered_map<long,bool> UnwantedTransactions;
    
    
    
    //-------------------------------------------------------------------------
    //Apriori Algorithm for frequent Itemset Begin!!:
    //-------------------------------------------------------------------------
    
    
    for(int k=2; KSize_Freq_ItmSet.size()!=0;k++ )
    {
        //Clearing the previous step's HashMap
        HashMap.clear();
        
        //candidates generated from Lk;
        GetCandidates(CandidateItemSet, KSize_Freq_ItmSet, KSize_Freq_HashMap, k);
        
        //Populating Candidate_HashMap For Fast Accessing the Candidate Itemset for the next step
        for (auto const& set : CandidateItemSet)
        {
            Candidate_HashMap.insert({set,true});
        }
        
        //for each transaction t in database do
        //increment the count of all candidates in
        IncrementHashMapCounter(HashMap, Candidate_HashMap,CandidateItemSet, k, UnwantedTransactions);
        
        //Clearing the Candidate Itemsets
        CandidateItemSet.clear();
        Candidate_HashMap.clear();
        
        //Clearing the K_Size Freq Itemsets
        KSize_Freq_ItmSet.clear();
        KSize_Freq_HashMap.clear();
        
        //Populating FreqItemSet
        for (auto &h : HashMap)
        {
            if(h.second >= Min_Support*Num_of_TotalTrans)
            {
                FreqItemSet.insert(h);
                KSize_Freq_ItmSet.push_back(h.first);
            }
        }
        
        //Populating KSize_Freq_HashMap
        for (auto const& set : KSize_Freq_ItmSet)
        {
            KSize_Freq_HashMap.insert({set,true});
        }
    }
    //Apriori Algorithm for finding frequent itemsets End
    
    //-------------------------------------------------------------------------
    //----------------Apriori Algorithm for frequent Itemset End!!:
    //-------------------------------------------------------------------------
    
    
    //Strong Associan Rules
    vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>> Strong_Assoc_Rules;
    
    //Outputing the Result Based On Output Parameter (4th Param)
    switch (Action) {
        case 'f': //Only Output the Freq. Itemset
            OutputFrequentItemSet(FreqItemSet);
            break;
            
        default:
            
            //-------------------------------------------------------------------------
            //----------------Generating String Association Rules!!:
            //-------------------------------------------------------------------------
            
            Association_Rule_Gen(Strong_Assoc_Rules, FreqItemSet);
            
            switch (Action) {
                    
                case 'r': //Only Output the Strong Rules
                    OutputStrongRules(Strong_Assoc_Rules);
                    break;
                    
                case 'a' : //Output both Strong Rules and Freq. Itemsets
                    OutputFrequentItemSet(FreqItemSet);
                    OutputStrongRules(Strong_Assoc_Rules);
                    break;
                    
                default:
                    end = clock();
                    elapsed_secs = double(end - begin) / CLOCKS_PER_SEC;
                    //If Non specified for Action:
                    //Output the Count measures and Execution Time
                    OutputResultCount(FreqItemSet, Strong_Assoc_Rules, elapsed_secs);
                    break;
            }
    }
    
    DB.close();
    return 0;
}




//////////////////////////////////////////////
//////////////////********END*********////////
//////////////////////////////////////////////




//candidates generated from Lk
void GetCandidates(vector<vector<long> >& CandidateItemSet, vector<vector<long> > KSize_Freq_ItmSet,std::map<vector<long>,bool> KSize_Freq_HashMap, int k)
{
    vector<long> candidate_elmnt;
    vector<long> temp_elmnt;
    vector<long> temp_FreqSet1;
    vector<long> temp_FreqSet2;
    
    long FreqSetSize=KSize_Freq_ItmSet.size();
    long it1,it2; //iteration cntrs
    
    bool found; //To check if The Lk-1 is  Found in K-1 Frequent Item Set or not
    bool pruned; //To check if the item should be pruned or not
    long varItem; // temp item
    int cntr;
    
    //Check if Two Sets Are Equall but only in one element
    bool Is_Diff_One;
    
    //Candidate Generation Begin!
    for( it1 = 0; it1<FreqSetSize; ++it1)
    {
        temp_FreqSet1= KSize_Freq_ItmSet[it1];
        
        for(it2 = it1+1; it2 < FreqSetSize; ++it2)
        {
            temp_FreqSet2= KSize_Freq_ItmSet[it2];
            
            //If The Sets only differ in one element => Could be a candidate!
            Is_Diff_One=true;
            
            for(cntr=0; cntr<k-2; cntr++)
            {
                if(temp_FreqSet1[cntr]!=temp_FreqSet2[cntr])
                {
                    Is_Diff_One=false;
                    break;
                }
            }
            
            //Check if the candidate itemset has infrequent subset
            if(Is_Diff_One)
            {
                //Candidate item set is the union of the two sets which have only one diff.
                if(k!=2)
                    candidate_elmnt.assign(temp_FreqSet1.begin(), temp_FreqSet1.end()-1);
                
                if(temp_FreqSet1[k-2]>temp_FreqSet2[k-2])
                {
                    candidate_elmnt.push_back(temp_FreqSet2[k-2]);
                    candidate_elmnt.push_back(temp_FreqSet1[k-2]);
                }
                else
                {
                    candidate_elmnt.push_back(temp_FreqSet1[k-2]);
                    candidate_elmnt.push_back(temp_FreqSet2[k-2]);
                }
                
                //Now The candidate element should be checked that has_infrequent_subset(c, Lk−1) ?
                
                pruned=false;
                for(vector<long>::iterator it3 = candidate_elmnt.begin(); it3 != candidate_elmnt.end(); ++it3)
                {
                    found=false;
                    
                    //check all of the Lk-1 Options
                    varItem=(*it3);
                    temp_elmnt=candidate_elmnt;
                    temp_elmnt.erase(remove(temp_elmnt.begin(), temp_elmnt.end(), varItem), temp_elmnt.end());
                    
                    //Check if The Lk-1 is  Found in K-1 Frequent Item Set or not!
                    std::map<vector<long>, bool>::const_iterator SearchKey = KSize_Freq_HashMap.find (temp_elmnt);
                    if ( SearchKey == KSize_Freq_HashMap.end() )
                    {
                        //The Lk-1 is not Found in K-1 Frequent Item Set
                        //So it Should be Pruned
                        pruned=true;
                        break;
                    }
                    
                    temp_elmnt.clear();
                }
                
                if(pruned==false)
                {
                    CandidateItemSet.push_back(candidate_elmnt);
                }
                candidate_elmnt.clear();
            }
        }
    }
    
}

//increment the count of all candidates in Ck+1 that are contained in transaction
void IncrementHashMapCounter(std::map<vector<long>,long>& HashMap, std::map<vector<long>,bool> Candidate_HashMap, const vector<vector<long> >& CandidateItemSet, int k, unordered_map<long,bool>& UnwantedTransactions)
{
    DB.clear();
    DB.seekg (0, DB.beg);
    
    string Transaction; //Considering each line of txt file as a transaction
    vector<long> Item; //Considering each split as an Item
    vector<vector<long> > TransSubsets;
    vector<long> TempSet;
    
    //To Prune Those Lines of the Input File (TRansactions) That does not have any candidates:
    bool Contributed;
    
    long TransID=0;
    unsigned long Itemsize;
    unsigned long CandidateSize= CandidateItemSet.size();
    unsigned long Cnt_Subset;
    bool Caindid_Existed;
    
    //IncrementHashMapCounter Begins
    while (getline(DB, Transaction))
    {
        TransID++;
        
        //Check if the Line is pruned previously or not!
        unordered_map<long,bool>::const_iterator TransIDIter = UnwantedTransactions.find (TransID);
        
        if ( TransIDIter == UnwantedTransactions.end() )
        {
            //We Assume That First, This Trnasaction Does not Contribute to the FReqItemSet
            Contributed=false;
            
            //Splitting the Transaction by Space
            //[Idea from cplusplus.com]
            istringstream buf(Transaction);
            istream_iterator<string> beg(buf), end;
            vector<string> tokens(beg, end);
            
            for(int tokenIndex=0; tokenIndex<tokens.size(); tokenIndex++)
            {
                Item.push_back(stol(tokens[tokenIndex])) ;
            }
            
            sort(Item.begin(),Item.end());
            vector<long> current;
            Itemsize=Item.size();
            
            if(Itemsize>=k)
            {
                Cnt_Subset=SubsetCount(Itemsize, k); //Cnt_Subset is the number of subsets of the Transaction
                
                //If the Size of the Candidates is less than the size of the Subsets of Item,
                //Then It is more efficient to just check all of the candidates instead of checking
                //all of the subsets.
                if(Cnt_Subset<=CandidateSize/50)//Checking All of the subsets
                {
                    //Creating All Subsets of the Transaction
                    SetSubsetsOfSizeK(TransSubsets, Item, k, 0, current);
                    
                    for(vector<vector<long> >::iterator it = TransSubsets.begin(); it != TransSubsets.end(); ++it)
                    {
                        
                        TempSet=(*it);
                        std::map<vector<long>, bool>::const_iterator SearchKey1 = Candidate_HashMap.find (TempSet);
                        if ( SearchKey1 != Candidate_HashMap.end() )
                        {
                            Contributed=true;
                            
                            std::map<vector<long>, long>::const_iterator SearchKey2 = HashMap.find (TempSet);
                            if ( SearchKey2 == HashMap.end() )
                            {
                                //If the Item Does not exist in HashMap, Then Insert it
                                HashMap.insert({TempSet,1});
                            }
                            else
                            {
                                //If the Item exists in HashMap, Then Add one to its Counter
                                HashMap[TempSet] = (SearchKey2->second)+1;
                            }
                        }
                    }
                    
                    TransSubsets.clear();
                }
                else
                {
                    //checking all of the candidates instead of checking
                    //all of the subsets.
                    
                    //Check If all of the elemets in the Candidate Items are present in the Transaction.
                    for (auto &Candid_Item : CandidateItemSet)
                    {
                        Caindid_Existed=true;
                        for (auto &elem : Candid_Item)
                        {
                            if ( find(Item.begin(), Item.end(), elem) == Item.end() )
                            {
                                Caindid_Existed=false;
                                break;
                            }
                        }
                        if(Caindid_Existed)
                        {
                            Contributed=true;
                            //If The Loop is Finished Then
                            //It means that The Transaction Supports the Candidate
                            //So it should
                            std::map<vector<long>, long>::const_iterator SearchKey2 = HashMap.find (Candid_Item);
                            if ( SearchKey2 == HashMap.end() )
                            {
                                //If the Item Does not exist in HashMap, Then Insert it
                                HashMap.insert({Candid_Item,1});
                            }
                            else
                            {
                                //If the Item exists in HashMap, Then Add one to its Counter
                                HashMap[Candid_Item] = (SearchKey2->second)+1;
                            }
                        }
                        
                    }
                    
                }
            }
            Item.clear();
            if(Contributed==false)
            {
                UnwantedTransactions.insert({TransID,true});
            }
        }
    }
}

//Populating the Vector containing all of the Subsets of size K
//[Idea from stackoverflow]
void SetSubsetsOfSizeK(vector<vector<long> >&  TransSubsets,vector<long> Item, int k, int idx, vector<long> current)
{
    //Output Result
    if (current.size() == k) {
        //sort(current.begin(), current.end());
        TransSubsets.push_back(current);
        return;
    }
    //Nothing, Return
    if (idx == Item.size())
    {return;}
    long x = Item[idx];
    current.push_back(x);
    //if X is in the subset
    SetSubsetsOfSizeK(TransSubsets, Item, k, idx+1, current);
    current.erase(remove(current.begin(), current.end(), x), current.end());
    //if X is not in the subset
    SetSubsetsOfSizeK(TransSubsets, Item, k, idx+1, current);
}

//Generating Association Rules
void Association_Rule_Gen(vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>> & Assoc_Rules, const std::map<vector<long>, long>& FreqItemSet)
{
    vector<long> FreqItem;
    vector<long> q; // q= ItemSet minus set;
    vector<vector<long>> Subsets;
    long FreqItem_Cnt; //Support For Set : FreqItem: p=>q
    long q_Cnt; // Support for q
    float Confidence;
    float ItemSupport;
    
    for (auto &h : FreqItemSet)
    {
        FreqItem=h.first;
        
        //All of the non-empty subsets of Freq. Item
        SetSubsets(Subsets, FreqItem);
        
        //Contructing p=>q in the sense that non of them are empty
        for (const auto& p: Subsets)
        {
            q= FreqItem;
            for (const auto& elem:p ) {
                q.erase(remove(q.begin(), q.end(), elem), q.end());
            }
            if(q.size()>0)
            {
                //Support [p Union q]
                FreqItem_Cnt= FreqItemSet.at(FreqItem);
                
                //Support q
                q_Cnt= FreqItemSet.at(p);
                
                //Conf= Support [p Union q] Divide by [Support q]
                Confidence=(1.0*FreqItem_Cnt)/(1.0*q_Cnt);
                
                if(Confidence>=Min_Confidence)
                {
                    ItemSupport=(1.0*FreqItem_Cnt)/(1.0*Num_of_TotalTrans);
                    Assoc_Rules.push_back({{p,q},{ItemSupport,Confidence}});
                    
                }
                
            }
            
        }
        Subsets.clear();
    }
}

//Finding all of the subsets of an ItemSet
//Idea From geeksforgeeks.org
void SetSubsets(vector<vector<long> >&  Subsets, vector<long> Item)
{
    
    
    unsigned long set_size = Item.size();
    
    //sort(Item.begin(), Item.end());
    unsigned long pow_set_size = pow(2, set_size);
    int counter, j;
    
    vector<long> TempItem;
    
    
    //Run from counter 000..0 to 111..1
    for(counter = 0; counter < pow_set_size; counter++)
    {
        for(j = 0; j < set_size; j++)
        {
            //Check if jth bit in the counter is set
            //If set then pront jth element from set
            if(counter & (1<<j))
            {
                TempItem.push_back(Item[j]);
            }
        }
        if(TempItem.size()>=1)
        {
            Subsets.push_back(TempItem);
        }
        TempItem.clear();
    }
    
}

//For Outputting all FrequentItemSet in Output
void OutputFrequentItemSet(const std::map<vector<long>, long>&  FreqItemSet)
{
    tempStrBuffer="";
    stringstream stream;
    
    //Output the Frequent_Item_Set
    //tempStrBuffer+="Frequent Item Set Size is "+FreqItemSet.size()+"\n";
    for (auto &h : FreqItemSet)
    {
        for (auto const& element : h.first)
        {
            tempStrBuffer+= to_string(element)+", " ;
        }
        
        //Cursor moves 2 position backwards
        tempStrBuffer.pop_back();
        tempStrBuffer.pop_back();
        stream<<fixed<<setprecision(2)<<((1.0*h.second)/(1.0*Num_of_TotalTrans));
        tempStrBuffer+=" ("+stream.str()+")";
        tempStrBuffer+="\n";
        stream.str("");
        
    }
    cout<<tempStrBuffer;
}

//For Outputting all Strong Rules in Output
void OutputStrongRules(const vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>>& Strong_Assoc_Rules)
{
    tempStrBuffer="";
    stringstream stream;
    
    //cout<<"Association_Rules Size is "<<Strong_Assoc_Rules.size()<<endl;
    for ( auto& Rule: Strong_Assoc_Rules) {
        for (const auto& p: Rule.first.first) {
            
            tempStrBuffer+=to_string(p)+", ";
        }
        tempStrBuffer.pop_back();
        tempStrBuffer.pop_back();
        //Cursor moves 2 position backwards
        
        tempStrBuffer+=" -> ";
        for (const auto& q: Rule.first.second) {
            tempStrBuffer+=to_string(q)+", ";
        }
        tempStrBuffer.pop_back();
        tempStrBuffer.pop_back();
        //Cursor moves 2 position backwards
        stream<<setprecision(2) << fixed <<" ("<<Rule.second.first<<", "<<Rule.second.second<<")";
        
        tempStrBuffer+=stream.str()+"\n";
        stream.str("");
    }
    cout<<tempStrBuffer;
}

//Outputting number of frequent itemsets of different sizes and the number of strong rules
void OutputResultCount(const std::map<vector<long>, long>&  FreqItemSet, const vector<pair<pair<vector<long>,vector<long>>,pair<float,float>>>& Strong_Assoc_Rules, double elapsed_secs)
{
    map<unsigned long, long> KSizeItemSetCounts;
    unsigned long ItemSize;
    
    //Populating KSizeItemSetCounts in the form that (Number of frequent K_itemsets, Count)
    for (auto &Item : FreqItemSet)
    {
        ItemSize=Item.first.size();
        map<unsigned long, long>::const_iterator SearchKey = KSizeItemSetCounts.find (ItemSize);
        if ( SearchKey == KSizeItemSetCounts.end() )
        {
            KSizeItemSetCounts.insert({ItemSize,1});
        }
        else
        {
            KSizeItemSetCounts[ItemSize] = (SearchKey->second)+1;
            
        }
    }
    
    //Output The KSizeItemSetCounts
    for (auto &Item : KSizeItemSetCounts)
    {
        cout<<"Number of frequent "<< Item.first<<"_itemsets: "<<Item.second<<endl;
    }
    
    //Output The Size of String Assoc Rules
    cout<<"Number of association rules  : "<< Strong_Assoc_Rules.size() <<endl;
    cout<< setprecision(2) << fixed <<"Execution Time               : "<<elapsed_secs<< " Seconds"<<endl;
}

//Calculating Choose Itemsize from k [Idea from StackOverflow]
unsigned long SubsetCount(unsigned long Itemsize,unsigned long k)
{
    if (k > Itemsize) return 0;
    if (k * 2 > Itemsize) k = Itemsize-k;
    if (k == 0) return 1;
    
    unsigned long result = Itemsize;
    for( int i = 2; i <= k; ++i ) {
        result *= (Itemsize-i+1);
        result /= i;
    }
    return result;
}

