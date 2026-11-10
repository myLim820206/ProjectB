// 스킬 트리 표를 JSON 으로 뽑는다 (tools/neon/neon.py tree 미리보기용)
//   g++ -std=c++17 Tests/tree_dump.cpp Source/ProjectB/PDCore.cpp Source/ProjectB/PDSynth.cpp -o /tmp/tree_dump && /tmp/tree_dump > tree.json
#include "../Source/ProjectB/PDCore.h"
#include <cstdio>
using namespace PD;
int main(){ std::printf("[");
 for(int i=0;i<TreeNodeCount();++i){auto&n=TreeNode(i); std::printf("%s{\"id\":%d,\"k\":%d,\"r\":%g,\"d\":%g,\"fx\":[[%d,%d,%g],[%d,%d,%g]],\"l\":[",i?",":"",i,(int)n.Kind,n.R,n.Deg,(int)n.Fx[0].Stat,n.Fx[0].Param,n.Fx[0].Value,(int)n.Fx[1].Stat,n.Fx[1].Param,n.Fx[1].Value);
 auto&L=TreeLinks(i); for(size_t j=0;j<L.size();++j)std::printf("%s%d",j?",":"",L[j]); std::printf("]}");}
 std::printf("]\n");}
