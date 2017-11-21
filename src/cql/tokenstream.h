#pragma once
#include "util.h"
#include "token.h"
#include "node.h"
    
  
class Tokens{
 public:
  void print();
  int bottom;
  vector<Token*>tokens;
  bool eof();
  Token* current();
  int save();
  void restore(int x);
  void forward();
  Tokens(vector<Token*> ts);

  Node* match_cqlfeature();
  StringToken* match_string();
  StringToken* match_string(char*);
  CqlNode* match_cqlnode();
  PgnNode* match_pgnnode();
  OutputNode* match_outputnode();
  ResultNode* match_resultnode();
  vector<Node*>* match_some_cqlfeatures();
  KeywordToken* match_keyword(const char*v);
  bool match_lparen();
  bool match_rparen();
  bool match_bar();
  Node* match_basicnode();
  Node* match_basicnode_no_or();
  Node* match_simplenode();
  PieceLoc* match_pieceloc();
  Node* match_compoundnode();
  Node* match_tagnode();
  PositionNode* match_positionnode();
  CountNode* match_countnode();
  Token* match_variable();
  vector<Node*> match_some_basicnodes();
  ForallNode* match_forallnode();
  Range* match_range();
  Range* match_compoundrange();
  IntToken* match_int();
  VariationsNode* match_variationsnode();
  GameNumberNode* match_gamenumbernode();
  AttackNode* match_attacknode();
  vector<SetBase*>* match_some_setbases();
  RayNode* match_raynode();
  KeywordToken* match_keyword();
  MoveBase*match_movebase();
  MoveNumberNode*match_movenumbernode();
  NotNode*match_notnode();
  KeywordToken*match_transformkeyword();
  KeywordToken*match_raykeyword();
  SetBase*match_setbase();
  static vector<directionT> directionsFromRayKeyword(KeywordToken*);
  //  bool match_lessthan();
  //  bool match_greaterthan();
  SequenceBase* match_sequencenode();
  SquareVariable* match_squarevariable();
  TagVariable* match_tagvariable();
  SetBase* match_parenthesizedsetbase();
  bool match_lbrace();
  bool match_rbrace();
  void show_error(const char* message);
  NotSetNode* match_notsetnode();
  VectorNode* match_vectornode();
  vector<DirectionParameter> match_directionparameters();
  DirectionParameter* match_directionparameter();
  vector<Direction>*match_direction();
  vector<Direction> match_raydirections(bool * isattack);
  GapNode* match_gapnode();
  TransformBase* match_fliptransform();
  TransformBase* match_shifttransform();
  TransformNode* match_transformnode();
  TransformBase* match_transformbase();
  TransformSetNode* match_transformsetnode();
  NumericVariable* match_numericvariable();
  Node* match_countable();
  MatchCountNode* match_matchcountnode();
  FutureStarNode* match_futurestarnode();
  PastStarNode* match_paststarnode();
  SetBase* match_inexpr();
  AnyNode* match_anynode();
  BetweenNode* match_betweennode();
  ExtensionNode* match_extensionnode();
  OnNode* match_onnode();
  SetBase*match_fromexp();
  SetBase*match_toexp();
  PieceLoc*match_promoteexp();
  SetBase*match_enpassantexp();
  EchoNode* match_echonode();
  EchoSpec* match_echospec();
  EchoSpec* match_echotransformspec();
  vector<EchoSpec*> match_echospecs();
  EchoMaxDistanceSpec* match_echomaxdistancespec();
  EchoSumDistanceSpec* match_echosumdistancespec();
  EchoTargetDistanceSpec* match_echotargetdistancespec();
  EchoSourceDistanceSpec* match_echosourcedistancespec();
  EchoLongestSubstringSpec* match_echolongestsubstringspec();
  bool match_echoemptyspec();
  EchoSquareSpec* match_echosquarespec();
  EchoDistanceSpec* match_echoancestor();
  EchoDistanceSpec* match_echodescendant();
  PowerNode* match_powernode();
  PowerDifferenceNode* match_powerdifferencenode();
  ExistsNode* match_existsnode();
  ExistsNode* match_ranged_existsnode(Range**);
  CountNode* match_count_existsnode();
  PieceIdNode* match_pieceidnode();
  PieceIdNode* match_ranged_pieceidnode(Range**);
  CountNode* match_count_pieceidnode();
  vector<Transform*>match_echotransforms();
  EchoSideToMoveSpec* match_echosidetomovespec();
  OriginNode* match_originnode();
  Node* match_orsuffix();
  SetBase* match_onsuffix();
  SetBase* match_primary_setbase();
  MatchCommentNode* match_matchcommentnode();
  const char* match_quotedstring();
  bool match_keywords(const char *v1, const char*v2);
  SilentFeatureNode* match_silentfeature();
  Node* match_silent();
  CommentBase* match_commentbase();
  Node* match_parenthesizedcountable();
  PlayerNode* match_playernode();
  EloNode* match_elonode();
  colorT match_color();
  YearNode* match_yearnode();
  EventNode* match_eventnode();
  SiteNode* match_sitenode();
  Node* match_sortbodynode();
  NumericVariable* match_optionalnumericvariable(bool ismax);
  SeqConstituent* match_seqconstituent();
  vector<SeqConstituent*>match_some_seqconstituents();
  bool match_star();
  bool match_questionmark();
  bool match_plus();
  HolderConstituent*match_holderconstituent();
  VectorConstituent*match_vectorconstituent();
  SeqConstituent*match_seqsuffix(SeqConstituent*c);
  bool match_keywordstar(const char* name);
  NumericVariable*match_sortheader();
};  
