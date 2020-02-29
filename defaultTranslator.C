/*	
        Add HLS debug suuport for OpenCl
        Author: Matthew Ashcraft
        Brigham Young University
        CCL Lab
        Based off of code:
        Add Event Observability Ports (EOP) to input C source code
        Author: Jose Pinilla
        University of British Columbia
        SoC Research Lab
*/
#include "rose.h"
#include <CallGraph.h>
#include <iostream>
#include <transformationSupport.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "interproceduralCFG.h"
#include <queue>
#include <sstream>
#include <string>

//#include "staticCFG.h"
using namespace SageInterface;
using namespace SageBuilder;
using namespace std;
using namespace Rose;

#define GLOBAL
//#define LEGUP
#define EOP

int main (int argc, char *argv[])
{
  int buffSize = 4096;
  string rootDir (argv[1]);
  size_t dir = rootDir.rfind("/");
  rootDir.replace(dir, rootDir.length()-dir, "/");
  //cout << "ROOT_DIR: " << rootDir << endl;
  string projectDir = rootDir;
  projectDir.replace(dir, projectDir.length()-dir, "/");
  //cout << "PROJECT_DIR: " << projectDir << endl;
  SgProject *project = frontend (argc, argv);
  SgGlobal * globalScope = getFirstGlobalScope (project);
  Sg_File_Info* openCLFile = NULL;
  
  //generatePDF ( *project );
  //generateDOT ( *project );
  //const int MAX_NUMBER_OF_IR_NODES_TO_GRAPH_FOR_WHOLE_GRAPH = 4000;
  //generateAstGraph(project,MAX_NUMBER_OF_IR_NODES_TO_GRAPH_FOR_WHOLE_GRAPH);
  
  std::set<string> dbgFuncName;
  string hlsDebugConfig = rootDir + "hlsDebugConfig.txt";
  ifstream file;
  file.open(hlsDebugConfig.c_str());
  while(file.good()){
    string str;
    file >> str;
    if(str == "KERNEL_NAME"){
      string func;
      file >> func;
      dbgFuncName.insert(func);
      //cout << func << endl;
    } else if (str == "BUFFER_SIZE"){
      file >> str;
      stringstream size(str);
      size >> buffSize;
    } else {
      //cout << "Unknown entry in hlsDebugConfig.txt: " << str << endl;
    }
  }
  file.close();
  if(dbgFuncName.empty()){
    dbgFuncName.insert("main");
  }
  //typedef boost::unordered_map<SgFunctionDeclaration *, SgGraphNode *> CGMapping;

  /*std::cout<<"Generating Input PDF"<<std::endl;

    AstPDFGeneration pdfIn;
    pdfIn.generate("inputCode",project);*/

  std::map<SgNode*, SgVariableSymbol*> nodeToVarSymbol;
  std::map<SgNode*, SgFunctionDefinition*> nodeToFunc;
  std::map<SgFunctionDefinition*, std::set<SgNode*> > funcToNodes;
  // Set of variables to insert unique ports
  std::set<SgVariableSymbol*> EOPS;
  std::set<SgGraphNode*> dbgFuncNodes;
  std::map<SgFunctionDeclaration*, SgGraphNode*> dbgFunc;
  std::map<SgNode*, SgFunctionDefinition*> nodesPerTopFunc;
  std::set<SgFunctionDefinition*> topFuncs;
  std::map<string, SgFunctionDefinition*> definedFuncs;
  std::map<unsigned int, SgNode*> valIDs;
  unsigned int valID = 1;
  std::set<SgNode*> getGlobalIDCall;
  std::set<SgNode*> getLocalIDCall;
  std::set<SgNode*> getGroupIDCall;
  std::map<SgFunctionDefinition*, SgNode*> funcToGlobalID;
  std::map<SgFunctionDefinition*, SgNode*> funcToLocalID;
  std::map<SgFunctionDefinition*, SgNode*> funcToGroupID;

  Rose_STL_Container<SgNode*> memberFunctionDefinitionList = NodeQuery::querySubTree (project,V_SgFunctionDefinition);
  for (Rose_STL_Container<SgNode*>::iterator i = memberFunctionDefinitionList.begin(); i != memberFunctionDefinitionList.end(); i++){
    SgFunctionDefinition* tmpFuncDef = isSgFunctionDefinition(*i);
    definedFuncs[tmpFuncDef->get_declaration()->get_name().str()] = tmpFuncDef;;
    if(dbgFuncName.count(tmpFuncDef->get_declaration()->get_name().str())){
      openCLFile = tmpFuncDef->get_file_info();
      topFuncs.insert(tmpFuncDef);
      StaticCFG::InterproceduralCFG cfg(tmpFuncDef);
      string fileName= StringUtility::stripPathFromFileName(tmpFuncDef->get_file_info()->get_filenameString());
      //string dotFileName1=fileName+"."+ tmpFuncDef->get_declaration()->get_name() +".IPCFG.dot";
      
      SgGraphNode* entry = NULL;
      std::map<CFGNode, SgGraphNode*> alNodes = cfg.alNodes;
      for(std::map<CFGNode, SgGraphNode*>::iterator node = alNodes.begin(),
            endNode = alNodes.end(); node != endNode; ++node){
        if(node->first.getNode() == cfg.getEntry()){
          entry = node->second;
          break;
        }
      }
      //process graph
      std::set<SgGraphNode*> graphNodes;
      std::queue<SgGraphNode*> toProcess;
      toProcess.push(entry);
      while (!toProcess.empty()){
        SgGraphNode* node = toProcess.front();
        SgNode* tmpNode = node->get_SgNode();
        toProcess.pop();
        if(graphNodes.count(node))
          continue;
        nodesPerTopFunc[tmpNode] = tmpFuncDef;
        graphNodes.insert(node);
        

        SgNode* parent = tmpNode;
        
        if(isSgFunctionDefinition(parent)){
          SgFunctionDefinition* def = isSgFunctionDefinition(parent);
          nodeToFunc[tmpNode] = def;
          funcToNodes[def].insert(tmpNode);
         
        } else {
          while(parent->get_parent()){
            parent = parent->get_parent();
            if(isSgFunctionDefinition(parent)){
              SgFunctionDefinition* def = isSgFunctionDefinition(parent);
              nodeToFunc[tmpNode] = def;
              funcToNodes[def].insert(tmpNode);
            }
          }
        }
        std::vector<SgDirectedGraphEdge*> edges = cfg.getOutEdges(node);
        for(std::vector<SgDirectedGraphEdge*>::iterator edge = edges.begin(),
              endEdge = edges.end(); edge != endEdge; edge++){
          SgDirectedGraphEdge* tmpEdge = *edge;
          SgGraphNode* outNode = tmpEdge->get_to();
          if(!graphNodes.count(outNode)){
            toProcess.push(outNode);
          }
        }
      }
      
    }
  }
  
  

   //======================================== OPERATION SEARCH =========================

  // Find all assignment operations we want to instrument
  std::vector<SgNode* > binOpList = NodeQuery::querySubTree (project, V_SgBinaryOp);
  std::vector<SgNode* > uniOpList = NodeQuery::querySubTree (project, V_SgUnaryOp);
  std::vector<SgNode* > iniOpList = NodeQuery::querySubTree (project, V_SgAssignInitializer); //TEST

  //std::cout<<"FOUND INIT "<<iniOpList.size()<<std::endl;

  std::vector<SgNode* > assignOpList;

  assignOpList.reserve(binOpList.size() + uniOpList.size() + iniOpList.size());
  assignOpList.insert( assignOpList.end(), binOpList.begin(), binOpList.end() );
  assignOpList.insert( assignOpList.end(), uniOpList.begin(), uniOpList.end() );
  assignOpList.insert( assignOpList.end(), iniOpList.begin(), iniOpList.end() );


  //========================================= LOOP OVER OPERATIONS ==============================
  
  std::vector<SgNode*>::iterator iter;
  std::map<SgNode*, SgExpression*> nodoToLHSExp;
  for (iter = assignOpList.begin(); iter!= assignOpList.end(); iter++) {
    SgNode * node = (*iter);
    SgNode* tmpIDNode = node;
    std::string tmpVarName = node->unparseToString();
    bool globalIDSet = false;
    if(tmpVarName.compare(0, 13, "get_global_id") == 0 ||
       tmpVarName.compare(1, 13, "get_global_id") == 0){
      getGlobalIDCall.insert(node);
      globalIDSet = true;
    }
    bool localIDSet = false;
    if(tmpVarName.compare(0, 12, "get_local_id") == 0 ||
       tmpVarName.compare(1, 12, "get_local_id") == 0){
      getLocalIDCall.insert(node);
      localIDSet = true;
    }
    bool groupIDSet = false;
    if(tmpVarName.compare(0, 12, "get_group_id") == 0 ||
       tmpVarName.compare(1, 12, "get_group_id") == 0){
      getGroupIDCall.insert(node);
      groupIDSet = true;
    }    SgType * type;
    //SgStatement * asmStmt;
    SgExpression * assignExp;
    switch(node->variantT()) {

    case V_SgAssignInitializer:

      if (SgInitializedName *initName = isSgInitializedName(node->get_parent())){
        if(SgDeclarationStatement *initDecl = initName->get_declaration()){
          if(SgVariableDeclaration *initVarDecl = isSgVariableDeclaration(initDecl)){
            if (initVarDecl->get_parent() != globalScope){
              node = initDecl;
              type = initName->get_type();
              //asmStmt = isSgStatement(initDecl);
              assignExp = buildVarRefExp(initName);
              break;
            }
          }
        }
      }
      continue;

    case V_SgCompoundAssignOp:
    case V_SgPlusPlusOp:
    case V_SgMinusMinusOp:
    case V_SgPlusAssignOp:
    case V_SgAssignOp:
      //Get type to use for EOP type
      type = isSgExpression(node)->get_type();
      //asmStmt = TransformationSupport::getStatement(node);
      //Get operand being assigned
      if (SgBinaryOp * binOp = isSgBinaryOp(node)){
        assignExp = binOp->get_lhs_operand();
      }else if (SgUnaryOp * uniOp = isSgUnaryOp(node)){
        assignExp = uniOp->get_operand();
      }else{
        continue;
      }

      break;
    default:
      continue;

    }

    if(globalIDSet | localIDSet | groupIDSet){
      nodoToLHSExp[tmpIDNode] = assignExp;
    }
    
    //int index = std::distance(assignOpList.begin(),iter);
    // std::cout<<index<<": "<<node->unparseToString()<<" ln:"<<node->get_file_info()->get_line()<<node->get_file_info()->get_filename()<<std::endl;

    //std::cout<<"TYPE FILTERING"<<std::endl;

    //Prune higher level operations by discarding them until variable reference operation is found
    if (isSgUnaryOp(assignExp)){
      //std::cout<<"UNARY OPERATION"<<std::endl;
      continue;
    }

    //Not instrumenting constants
    if (isConstType(type)){
      //std::cout<<"CONST TYPE"<<std::endl;
      continue;
    }

    //TODO: Temporarily stop from recording pointers
    if (isSgPointerType(type)){
      //std::cout<<"POINTER TYPE"<<std::endl;
      //continue;
    }

    // Add variable symbol to set
    std::set<SgVariableSymbol*>::iterator varIter;
    std::pair<std::set<SgVariableSymbol*>::iterator, bool> ret;

    ostringstream osEOP;
    SgVariableSymbol * varSym;
    int declLine;
    if (SgVarRefExp * varRef = isSgVarRefExp(assignExp)){
      //std::cout<<"Normal Expression"<<std::endl;
      varSym = varRef->get_symbol();
    }else if(SgBinaryOp * binOp = isSgPntrArrRefExp(assignExp)){
      //TODO: TEMPORARY TEST
      //continue;
      //std::cout<<"Pointer Expression"<<std::endl;
      SgExpression* tempExp = binOp->get_lhs_operand();
      //If still a Binary Operation
      if (SgBinaryOp* bin_op = isSgBinaryOp(tempExp))
        //TODO: temprary fix for multi-dimensional arrays
        //FIX should store info of index and avoid read of data written to array.
        varSym = isSgVarRefExp(bin_op->get_lhs_operand())->get_symbol();
      else
        varSym = isSgVarRefExp(tempExp)->get_symbol();
    }else{
      //std::cout<<"Unrecognized Expression"<<std::endl;
      continue;
    }
    ret = EOPS.insert(varSym);
    declLine = varSym->get_declaration()->get_file_info()->get_line();
    osEOP << "eop_" << declLine << "_" << varSym->get_name().getString() << "_" << node->get_file_info()->get_line();

    //std::cout<<"SYMBOL: "<<osEOP.str()<<std::endl;
    if (SgAssignOp *assnOp = isSgAssignOp(node)){
      if (isSgValueExp(assnOp->get_rhs_operand())){
        //std::cout<<"CONSTANT VALUE"<<std::endl;
        continue;
      }
    }
    
    nodeToVarSymbol[node] = varSym;
    nodoToLHSExp[node] = assignExp;

    
   
  }
  
  std::map<SgFunctionDefinition*, std::vector<SgNode*> > sortedInstNodesByFunc;
  
  for(  std::map<SgFunctionDefinition*,
          std::set<SgNode*> >::iterator
          func = funcToNodes.begin(), endFunc= funcToNodes.end();
        func != endFunc; ++func){
    SgFunctionDefinition* tmpFunc = func->first;
    bool globalFound = false;
    bool localFound = false;
    bool groupFound = false;
    for(std::set<SgNode*>::iterator
          inst = func->second.begin(), endInst = func->second.end();
        inst != endInst; ){
      SgNode* tmpNode = *inst;
      ++inst;
      if(!globalFound && getGlobalIDCall.count(tmpNode)){
	funcToGlobalID[tmpFunc] = tmpNode;
	globalFound = true;
      }
      if(!localFound && getLocalIDCall.count(tmpNode)){
	funcToLocalID[tmpFunc] = tmpNode;
	localFound = true;
      }
      if(!groupFound && getGroupIDCall.count(tmpNode)){
	funcToGroupID[tmpFunc] = tmpNode;
	groupFound = true;
      }
      
      if(nodeToVarSymbol.count(tmpNode)){
	
        sortedInstNodesByFunc[tmpFunc].push_back(tmpNode);
      } else {
        continue;
      } 
    }
  }
    
  for(std::map<SgFunctionDefinition*, std::vector<SgNode*> >::iterator
        func = sortedInstNodesByFunc.begin(),
        endFunc = sortedInstNodesByFunc.end();
      func != endFunc; ++func){
      
    for(unsigned int i = 0; i < func->second.size(); i++){
      for(unsigned int j = func->second.size() -1; j > i; j--){
        if(func->second[j]->get_file_info()->get_line() <
           func->second[j-1]->get_file_info()->get_line()){
          SgNode* tmp = func->second[j];
          func->second[j] = func->second[j-1];
          func->second[j-1] = tmp;
        }
      }
    }
  }
  
  string hlsDebugVariableList = projectDir + "hlsDebugVariableList.txt";
  ofstream varList;
  varList.open(hlsDebugVariableList.c_str());
  for(  std::map<SgFunctionDefinition*,
          std::vector<SgNode*> >::iterator
          func = sortedInstNodesByFunc.begin(),
          endFunc= sortedInstNodesByFunc.end();
        func != endFunc; ++func){
    SgFunctionDefinition* tmpFunc = func->first;
    varList << tmpFunc->get_declaration()->get_name().getString() << ":"
            << tmpFunc->get_file_info()->get_filename() << ":"
            << tmpFunc->get_file_info()->get_line()
            << endl;
    bool first = true;
    for(std::vector<SgNode*>::iterator
          inst = func->second.begin(), endInst = func->second.end();
        inst != endInst; ){
      SgNode* tmpNode = *inst;
      ++inst;
      if(!first)
        varList << ", ";      
      else
        first = false;
      SgVariableSymbol* tmpVar = nodeToVarSymbol[tmpNode];
      varList << tmpVar->get_name().getString() << ":"
              << tmpNode->get_file_info()->get_line() << ":"
	      << valID;
      valIDs[valID] = tmpNode;
      valID++;
      
    }
    varList << endl << endl;

  }
  varList.close();

  //Call python gui
  string systemCall = "python3 ~/rose/gui/gui.py " + projectDir; 
  system(systemCall.c_str());

  std::set<SgNode*> nodeToRecord;
  std::set<unsigned int> IDToRecord;
  string hlsDebugSelectedVar = rootDir + "hlsDebugSelectedVar.txt";
  file.open(hlsDebugSelectedVar.c_str());
  string comma = ",";
  string col = ":";
  //  SgFunctionDefinition* curFunc = NULL;
  while(!file.eof()){
    string str;
    file >> str;
    stringstream tmpInt(str);
    unsigned int ID = 0;
    tmpInt >> ID;
    if(valIDs.count(ID)){
      IDToRecord.insert(ID);
    } else {
      //cout << "Unrecognized ID: " << ID << endl; 
    }
    
  
  }
  file.close();
  //cout << "openCLFile: " << openCLFile->get_filename() << endl;
  if(IDToRecord.empty())
    return 0;


  //Construct a call Graph
  CallGraphBuilder CGBuilder(project);
  CGBuilder.buildCallGraph(builtinFilter());
  SgIncidenceDirectedGraph* graph = CGBuilder.getGraph();
  boost::unordered_map<SgFunctionDeclaration*, SgGraphNode*> tmpGraphNodes = CGBuilder.getGraphNodesMapping();
  // Output to a dot file
  //AstDOTGeneration dotgen;
  //dotgen.writeIncidenceGraphToDOTFile( CGBuilder.getGraph(), rootDir +"_callGraph.dot");
  
  
  std::map<SgFunctionDefinition*, SgGraphNode*> graphNodes;
  for(boost::unordered_map<SgFunctionDeclaration*, SgGraphNode*>::iterator
	gn = tmpGraphNodes.begin(), endNode = tmpGraphNodes.end();
      gn != endNode; gn++){
    if(definedFuncs.count(gn->first->get_name().str())){
      graphNodes[definedFuncs[gn->first->get_name().str()]] = gn->second;
    }
  }


  std::queue<SgGraphNode*> toProcess;
  std::set<SgGraphNode*> calledFunctions;
  for(std::set<SgFunctionDefinition*>::iterator tFunc = topFuncs.begin(),
	endFunc = topFuncs.end(); tFunc != endFunc; ++tFunc){
    std::vector<SgGraphNode*> tmpVec;
    SgFunctionDefinition* tFuncDef = *tFunc;
    graph->getSuccessors(graphNodes[tFuncDef],tmpVec);
    if(!tmpVec.empty()){
      for(std::vector<SgGraphNode*>::iterator vecItr = tmpVec.begin(),
	    endVec = tmpVec.end(); vecItr != endVec; ++vecItr){
	toProcess.push(*vecItr);
      }
    }
  }
  while(!toProcess.empty()){
    SgGraphNode* top = toProcess.front();
    toProcess.pop();
    calledFunctions.insert(top);
    std::vector<SgGraphNode*> tmpVec;
    graph->getSuccessors(top,tmpVec);
    if(!tmpVec.empty()){
      for(std::vector<SgGraphNode*>::iterator vecItr = tmpVec.begin(),
	    endVec = tmpVec.end(); vecItr != endVec; ++vecItr){
	if(!calledFunctions.count(*vecItr)){
	  toProcess.push(*vecItr);
	}
      }
    }
  }
  std::set<SgFunctionDefinition*> addFuncArgs;
  for(std::map<SgFunctionDefinition*, SgGraphNode*>::iterator
	GNItr = graphNodes.begin(), endGN = graphNodes.end();
      GNItr != endGN; ++GNItr){
    if(calledFunctions.count(GNItr->second)){
      addFuncArgs.insert(GNItr->first);
    }
  }

  for(std::set<SgFunctionDefinition*>::iterator funcItr = addFuncArgs.begin(),
	endFunc = addFuncArgs.end(); funcItr != endFunc; ++funcItr){
    SgFunctionDefinition* func = *funcItr;
    ostringstream buffName;
    buffName << "argTraceBufferArray";
    ostringstream idxName;
    idxName  << "traceBufferIdx";
    ostringstream globalIndexName;
    globalIndexName << "traceBufferThreadID";

    SgType* ptrType = SgPointerType::createType(SgTypeLong::createType());
    SgType* intPtrType = SgPointerType::createType(SgTypeInt::createType());
  
    SgScopeStatement* functionScope = getScope(func);
    pushScopeStack(functionScope);
    SgFunctionDeclaration* fDec = func->get_declaration();
    SgFunctionParameterList* pList = fDec->get_parameterList();
    
    SgInitializedName* traceArrayArg = new SgInitializedName(buffName.str(), ptrType, NULL, fDec, NULL);


    fDec->append_arg(traceArrayArg);
    traceArrayArg->set_parent(pList);
    traceArrayArg->set_scope(func);

    SgInitializedName* traceOffsetArg = new SgInitializedName(idxName.str(), intPtrType, NULL, fDec, NULL);


    fDec->append_arg(traceOffsetArg);
    traceOffsetArg->set_parent(pList);
    traceOffsetArg->set_scope(func);
    SgInitializedName* traceThreadIDArg = new SgInitializedName(globalIndexName.str(), intPtrType, NULL, fDec, NULL);


    fDec->append_arg(traceThreadIDArg);
    traceThreadIDArg->set_parent(pList);
    traceThreadIDArg->set_scope(func);
    popScopeStack();
    /*
    func->get_body()->prepend_statement
      (buildAssignStatement(buildVarRefExp("int traceBufferIdx"),
			    buildVarRefExp("*traceBufferIdx")));
    func->get_body()->prepend_statement
      (buildAssignStatement(buildVarRefExp("int traceBufferThreadID"),
			    buildVarRefExp("*traceBufferThreadID")));
    */
  }
  

  //Add parameters to function calls
  std::vector<SgNode*> functionCallList = NodeQuery::querySubTree (project,V_SgFunctionCallExp);
  for (std::vector<SgNode*>::iterator i = functionCallList.begin(); i != functionCallList.end(); i++){
    SgFunctionCallExp* functionCall = isSgFunctionCallExp(*i);
    if(functionCall != NULL){
      SgFunctionDeclaration* tmpDec = isSgFunctionDeclaration(functionCall->getAssociatedFunctionDeclaration());
      if(tmpDec != NULL){
	for(std::set<SgFunctionDefinition*>::iterator
	      funcItr = addFuncArgs.begin(),
	      endFunc = addFuncArgs.end(); funcItr != endFunc; ++funcItr){

	  SgFunctionDefinition* tmpDef = *funcItr;
	  if(tmpDef->get_declaration()->get_name() == tmpDec->get_name()){
	    if(topFuncs.count(nodeToFunc[functionCall])){
	      functionCall->append_arg(buildVarRefExp("traceBufferArray"));
	      
	      
	    } else {
	      functionCall->append_arg(buildVarRefExp("argTraceBufferArray"));

	      
	    }
	    functionCall->append_arg(buildVarRefExp("&traceBufferIdx"));
	    functionCall->append_arg(buildVarRefExp("&traceBufferThreadID"));
	    break;
	  }
	}
      }      
    }
  }

  
  
  
  //if(getGlobalIDCall == NULL && getLocalIDCall != NULL)
  //  getGlobalIDCall = getLocalIDCall;
  
  //Insert recording instruction
  for(std::set<unsigned int>::iterator IDItr = IDToRecord.begin(),
        endID = IDToRecord.end(); IDItr != endID; IDItr++){
    //for(std::set<SgNode*>::iterator nodeItr = nodeToRecord.begin(),
    //    endNode = nodeToRecord.end(); nodeItr != endNode; nodeItr++){
    //SgNode* node = *nodeItr;
    unsigned int ID = *IDItr;
    stringstream IDstr;
    IDstr << ID;
    SgNode* node = valIDs[ID];
    SgExpression* lhsExp = nodoToLHSExp[node];
    //SgType* type = lhsExp->get_type();

    
    ostringstream os;
    ostringstream osIdx;
    ostringstream osIdxInc;
    
    std::string topFuncName =
      nodesPerTopFunc[node]->get_declaration()->get_name().str();
    								   
    if(topFuncs.count(nodeToFunc[node])){
      os << "traceBufferArray[ ";
      if(funcToGlobalID.count(nodesPerTopFunc[node]) ||
	 funcToLocalID.count(nodesPerTopFunc[node]) ||
	 funcToGroupID.count(nodesPerTopFunc[node]))
	os << "traceBufferThreadID * " << buffSize << " + ";
      os << "(" 
	 << "traceBufferIdx | " << (buffSize - 1) << ")]";
    
      osIdx  << "traceBufferIdx";
      osIdxInc  << "traceBufferIdx + 1";
    } else {
      os << "argTraceBufferArray[ ";
      os << "*traceBufferThreadID * " << buffSize << " + ";
      os << "(" 
	 << "*traceBufferIdx | " << (buffSize - 1) << ")]";
    
      osIdx  << "*traceBufferIdx";
      osIdxInc  << "*traceBufferIdx + 1";
      
      
    }

    //INSERT ASSIGNMENT TO EOP USING SCOPESTACK
    SgScopeStatement * insertScope;
    //If assignment inside "for" statement
    SgNode * parentNode = node->get_parent();
    if (isSgForInitStatement(parentNode->get_parent())){
      //std::cout<<"ERROR: In For Init"<<std::endl;
      continue;
    }
    else if(isSgForStatement(parentNode)){
      SgStatement * forBody = isSgForStatement(parentNode)->get_loop_body();
      insertScope = isSgScopeStatement(forBody);
      //TODO: Temporary action. Bypassing all iterators
      //continue;
      if (insertScope){
        pushScopeStack (insertScope);
        SgExprStatement* incIdx1 =
          buildAssignStatement(buildVarRefExp(osIdx.str()),
                               buildVarRefExp(osIdxInc.str()));
        SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),incIdx1);
        SgExprStatement* recID =
          buildAssignStatement(buildVarRefExp(os.str()),
			       buildVarRefExp(IDstr.str()));
	SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),recID);
	SgExprStatement* incIdx =
          buildAssignStatement(buildVarRefExp(osIdx.str()),
                               buildVarRefExp(osIdxInc.str()));
        SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),incIdx);
        SgExprStatement* addedEOP =
          buildAssignStatement(buildVarRefExp(os.str()), lhsExp);
        appendStatement(addedEOP);
        
	//std::cout<<"Added Node Is:  "<<addedEOP->unparseToString()<<std::endl;
        //std::cout<<"Added Node Is:  "<<incIdx->unparseToString()<<std::endl;
	
      } else { //FIX: For ROSE Bug not recognizing for-loops without braces as BasicBlocks
        insertScope = forBody->get_scope();
        pushScopeStack (insertScope);
	SgExprStatement* incIdx1 =
          buildAssignStatement(buildVarRefExp(osIdx.str()),
                               buildVarRefExp(osIdxInc.str()));
        SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),incIdx1);
        SgExprStatement* recID =
          buildAssignStatement(buildVarRefExp(os.str()), 
			       buildVarRefExp(IDstr.str()));
        SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),recID);
	SgExprStatement* incIdx =
	  buildAssignStatement(buildVarRefExp(osIdx.str()),
			       buildVarRefExp(osIdxInc.str()));
	SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),incIdx);
	SgExprStatement* addedEOP =
          buildAssignStatement(buildVarRefExp(os.str()), lhsExp);
        appendStatement(addedEOP);
        //std::cout<<"Added Node Is:  "<<addedEOP->unparseToString()<<std::endl;
        //std::cout<<"Added Node Is:  "<<incIdx->unparseToString()<<std::endl;
     
      }
  
      popScopeStack();
      insertScope->associatedScopeName(); //DEBUG printing
 
    }else{
      insertScope = TransformationSupport::getStatement(node)->get_scope();
      pushScopeStack (insertScope);
      SgExprStatement* incIdx1 =
          buildAssignStatement(buildVarRefExp(osIdx.str()),
                               buildVarRefExp(osIdxInc.str()));
        SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),incIdx1);
        SgExprStatement* recID =
          buildAssignStatement(buildVarRefExp(os.str()), 
			       buildVarRefExp(IDstr.str()));
      SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),recID);
      SgExprStatement* incIdx =
        buildAssignStatement(buildVarRefExp(osIdx.str()),
                             buildVarRefExp(osIdxInc.str()));
      SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),incIdx);
      SgExprStatement* addedEOP =
        buildAssignStatement(buildVarRefExp(os.str()), lhsExp);
      //appendStatement(addedEOP);
      SageInterface::insertStatementAfter(TransformationSupport::getStatement(node),addedEOP);

      //std::cout<<"Added Node Is:  "<<addedEOP->unparseToString()<<std::endl;
      //std::cout<<"Added Node Is:  "<<incIdx->unparseToString()<<std::endl;
      popScopeStack();
    }
    
  }

 
  
  //SgScopeStatement* openCLScope = getGlobalScope(openCLFile);
  for(std::set<SgFunctionDefinition*>::iterator funcItr = topFuncs.begin(),
        endFunc = topFuncs.end(); funcItr != endFunc; funcItr++){
    SgFunctionDefinition* func = *funcItr;

    
    ostringstream os;
    std::string topFuncName =
     func->get_declaration()->get_name().str();
    
     os << "traceBufferArray[ ";
    if(funcToGlobalID.count(func) ||
       funcToLocalID.count(func) ||
       funcToGroupID.count(func))
      os << "traceBufferThreadID * "<< buffSize << " + ";
  
    os << "("
       << "traceBufferIdx | " << (buffSize - 1) << ")]";
    ostringstream buffName;
    buffName << "traceBufferArray";
    ostringstream idxName;
    idxName  << "traceBufferIdx";
    ostringstream globalIndexName;
    globalIndexName << "traceBufferThreadID";
      
    SgType* ptrType = SgPointerType::createType(SgTypeLong::createType());
    //SgType* intPtrType = SgPointerType::createType(SgTypeInt::createType());
    SgScopeStatement* functionScope = getScope(func);
    pushScopeStack(functionScope);
    SgFunctionDeclaration* fDec = func->get_declaration();
    SgFunctionParameterList* pList = fDec->get_parameterList();

    
    SgInitializedName* traceArrayArg = new SgInitializedName(buffName.str(), ptrType, NULL, fDec, NULL);


    fDec->append_arg(traceArrayArg);
    traceArrayArg->set_parent(pList);
    traceArrayArg->set_scope(func);

    popScopeStack();
    func->get_body()->append_statement
      (buildAssignStatement(buildVarRefExp(os.str()),
                            buildVarRefExp(idxName.str())));
    
    func->get_body()->append_statement
      (buildAssignStatement(buildVarRefExp(idxName.str()),
                            buildVarRefExp(idxName.str() + " + 1")));
    
    func->get_body()->append_statement
      (buildAssignStatement(buildVarRefExp(os.str()),
                            buildVarRefExp("0")));

    string decIdx = "int " + idxName.str();
    func->get_body()->prepend_statement
      (buildAssignStatement(buildVarRefExp(decIdx),
			    buildVarRefExp("0")));
    string globalIDDec = "int " + globalIndexName.str();
	
    if(funcToGlobalID.count(func) ||
       funcToLocalID.count(func) ||
       funcToGroupID.count(func)){
      int tmp = funcToGlobalID.count(func) + funcToLocalID.count(func) +
	funcToGroupID.count(func);
      if(tmp == 1){
	SgNode* IDCall = NULL;
	if(funcToGlobalID.count(func))
	  IDCall = funcToGlobalID[func];
	else if (funcToLocalID.count(func))
	  IDCall = funcToLocalID[func];
	else
	  IDCall = funcToGroupID[func];
	//SgExpression* globalIDExpr = nodoToLHSExp[IDCall];
	string globalStr = IDCall->unparseToString();
	func->get_body()->prepend_statement
	  (buildAssignStatement(buildVarRefExp(globalIDDec),
				buildVarRefExp(globalStr)));
      } else {
	SgNode* globalIDCall = NULL;
	SgNode* localIDCall = NULL;
	SgNode* groupIDCall = NULL;
	string globalStr;
	bool first = true;
	if(funcToGlobalID.count(func)){
	  globalIDCall = funcToGlobalID[func];
	  first = false;
	  globalStr = globalIDCall->unparseToString();
	}
	if (funcToLocalID.count(func)){
	  localIDCall = funcToLocalID[func];
	  if(first){
	    globalStr = localIDCall->unparseToString();
	    first = false;
	  } else {
	    globalStr += " + " + localIDCall->unparseToString();
	  }
	}
	if (funcToGroupID.count(func)){
	  groupIDCall = funcToGroupID[func];
	  globalStr += " + " + groupIDCall->unparseToString();

	}
	func->get_body()->prepend_statement
	  (buildAssignStatement(buildVarRefExp(globalIDDec),
				buildVarRefExp(globalStr)));
      }
    } else {
      func->get_body()->prepend_statement
	(buildAssignStatement(buildVarRefExp(globalIDDec),
			      buildVarRefExp("0")));
    }
    // popScopeStack();
    
  }

  
 
  //============================= OUTPUT GENERATION ============================

  //std::cout<<"Generating PDF"<<std::endl;

  //AstPDFGeneration pdf;
  //pdf.generateInputFiles(project);
  
  //std::cout<<"Running Tests"<<std::endl;
  //AstTests::runAllTests(project);

  //std::cout<<"Multiple files"<<std::endl;
  SgFilePtrList filelist = project->get_files();
  SgFilePtrList::iterator fileIter= filelist.begin();
  for (;fileIter!=filelist.end();fileIter++) {
    SgFile* sfile = isSgFile(*fileIter);
    if (sfile != NULL){
      //std::cout<<"File "<<sfile->getFileName()<<std::endl;
    }
  }

  //std::cout<<"Unparsing"<<std::endl;
    
  project->unparse();
  
  /*std::cout<<"Unparsing and Compiling"<<std::endl;
    return backend(project);*/
}

