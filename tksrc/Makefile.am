
#
# Written by Richard Rogers <rprogers@uw.edu>
#
# Copyright (C) 2010 Jeff Bilmes
# Licensed under the Open Software License version 3.0
# See COPYING or http://opensource.org/licenses/OSL-3.0
#

ACLOCAL_AMFLAGS = -I m4

if HAVE_HG
# this will keep the Mercurial revision id current if you're building 
# in a working directory. If you're not, use the distributed hgstamp.h
VERSIONFLAG = -DHGID="\"`hg id`\"" -DHGDATE="\"`hg parents --template '{date|date}'`\""
else
VERSIONFLAG = 
endif


DEBUGFLAGS =

if CYGWIN_FLAGS
  ANSI = 
else
  ANSI = -ansi -pedantic
endif

# If configure determines we're using GCC C/C++ compilers, set flags appropriately
if USE_GCC_FLAGS
# Only used for GCC compilers
  OPTFLAGS = -O3
# extra optimization options for certain GMTK sources, used below.
  XOPTFLAGS = -funroll-loops -fargument-noalias-global
  GCC_FLAGS = -Wall $(ANSI)
else
# unknown compiler
  OPTFLAGS =
  XOPTFLAGS =
  GCC_FLAGS =
endif

DBFLAGS =
if NO_ASSERTIONS
DBFLAGS += -DNDEBUG
endif
if NO_MODEL_CHECKS
DBFLAGS += -DNMODELCHECK
endif


AM_CPPFLAGS = \
-I$(srcdir)/../miscSupport \
-I$(builddir)/../IEEEFloatingpoint \
-I$(srcdir)/../featureFileIO \
-I$(srcdir)/../deepMLP \
$(VERSIONFLAG) $(DBFLAGS) \
-DBINDIR="\"$(bindir)\""
# make BINDIR available for warning message in GMTK_FileParser.cc for ticket 320

AM_CFLAGS = $(DEBUGFLAGS) $(OPTFLAGS) $(GCC_FLAGS)
AM_CXXFLAGS = $(DEBUGFLAGS) $(OPTFLAGS) $(GCC_FLAGS)
AM_LDFLAGS = 

if MAKE_BUNDLE
# Must be OSX, diddle the flags
  AM_CFLAGS += -m32
  AM_CXXFLAGS += -m32
  AM_LDFLAGS += -m32
  OSXLDFLAGS = -dylib_file /System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib:/System/Library/Frameworks/OpenGL.framework/Versions/A/Libraries/libGL.dylib
else
  OSXLDFLAGS =
endif


clean-local:
	-rm -rf gmtkViz.app



LDADD = libGMTK.a libXOPT.a \
$(builddir)/../featureFileIO/libgmtkio.a \
$(builddir)/../miscSupport/libmiscSupport.a \
$(builddir)/../IEEEFloatingpoint/libIEEEsupport.a

# should not be necessary -- flex sources should be such that no -lfl is required
#LIBS += $(LEXLIB)

noinst_LIBRARIES = libXOPT.a libGMTK.a

libGMTK_a_SOURCES = \
hgstamp.h \
GMTK_Dense1DPMF.h GMTK_Dense1DPMF.cc \
GMTK_Sparse1DPMF.h GMTK_Sparse1DPMF.cc \
GMTK_MeanVector.h GMTK_MeanVector.cc \
GMTK_DiagCovarVector.h GMTK_DiagCovarVector.cc \
GMTK_RealMatrix.h GMTK_RealMatrix.cc \
GMTK_DoubleMatrix.h GMTK_DoubleMatrix.cc \
GMTK_DlinkMatrix.h GMTK_DlinkMatrix.cc \
GMTK_DirichletTable.h GMTK_DirichletTable.cc \
GMTK_DirichletPrior.h GMTK_DirichletPrior.cc \
GMTK_MDCPT.h GMTK_MDCPT.cc \
GMTK_MSCPT.h GMTK_MSCPT.cc \
GMTK_RealArray.h GMTK_RealArray.cc \
GMTK_CPT.h GMTK_CPT.cc \
GMTK_MixtureCommon.h GMTK_MixtureCommon.cc \
GMTK_Component.h GMTK_Component.cc \
GMTK_GaussianComponent.h GMTK_GaussianComponent.cc \
GMTK_GMParms.h GMTK_GMParms.cc \
GMTK_FileParser.h GMTK_FileParser.cc \
GMTK_FileTokenizer.ll \
GMTK_MTCPT.h GMTK_MTCPT.cc \
GMTK_Mixture.h GMTK_Mixture.cc \
GMTK_DiagGaussian.h GMTK_DiagGaussian.cc \
GMTK_Dlinks.h GMTK_Dlinks.cc \
GMTK_EMable.h GMTK_EMable.cc \
GMTK_LinMeanCondDiagGaussian.h GMTK_LinMeanCondDiagGaussian.cc \
GMTK_RngDecisionTree.h GMTK_RngDecisionTree.cc \
GMTK_NameCollection.h GMTK_NameCollection.cc \
GMTK_GMTemplate.h GMTK_GMTemplate.cc \
GMTK_JunctionTree.h GMTK_JunctionTree.cc \
GMTK_CountIterator.h \
GMTK_GraphicalModel.h GMTK_GraphicalModel.cc \
GMTK_MaxClique.h GMTK_MaxClique.cc \
GMTK_BoundaryTriangulate.h GMTK_BoundaryTriangulate.cc \
GMTK_Timer.h GMTK_Timer.cc \
GMTK_Signals.h GMTK_Signals.cc \
GMTK_PackCliqueValue.h GMTK_PackCliqueValue.cc \
GMTK_Vocab.h GMTK_Vocab.cc \
GMTK_NGramCPT.h GMTK_NGramCPT.cc \
GMTK_FNGramCPT.h GMTK_FNGramCPT.cc \
GMTK_RV.h GMTK_RV.cc \
GMTK_ContRV.h GMTK_ContRV.cc \
GMTK_DiscRV.h GMTK_DiscRV.cc \
GMTK_HidDiscRV.h GMTK_HidDiscRV.cc \
GMTK_ObsDiscRV.h GMTK_ObsDiscRV.cc \
GMTK_ObsContRV.h GMTK_ObsContRV.cc \
GMTK_ScPnShRV.h GMTK_ScPnShRV.cc \
GMTK_SwDiscRV.h GMTK_SwDiscRV.cc \
GMTK_SwRV.h GMTK_SwRV.cc \
GMTK_Sw_HidDiscRV.h GMTK_Sw_HidDiscRV.cc \
GMTK_Sw_ObsContRV.h GMTK_Sw_ObsContRV.cc \
GMTK_Sw_ObsDiscRV.h GMTK_Sw_ObsDiscRV.cc \
GMTK_VECPT.h GMTK_VECPT.cc \
GMTK_DeepNN.h GMTK_DeepNN.cc \
GMTK_DeepVECPT.h GMTK_DeepVECPT.cc \
GMTK_DeepCPT.h GMTK_DeepCPT.cc \
GMTK_NetworkFlow.h GMTK_NetworkFlow.cc \
GMTK_NetworkFlowTriangulate.h GMTK_NetworkFlowTriangulate.cc \
GMTK_LatticeADT.h GMTK_LatticeADT.cc GMTK_LatticeADT.h GMTK_LatticeNodeCPT.h \
GMTK_CFunctionDeterministicMappings.cc GMTK_CFunctionDeterministicMappings.h \
GMTK_FileTokenizer.cc \
GMTK_LatticeNodeCPT.cc \
GMTK_ScPnSh_HidDiscRV.cc \
GMTK_WeightMatrix.cc \
GMTK_NamedObject.h \
GMTK_Arguments.h \
GMTK_DiscRVType.h \
GMTK_FactorInfo.h \
GMTK_JunctionTreeNBestVals.h \
GMTK_NamedObject.h \
GMTK_ProgramDefaultParms.h \
GMTK_ScPnSh_HidDiscRV.h \
GMTK_ScPnSh_ObsContRV.h \
GMTK_ScPnSh_ObsDiscRV.h \
GMTK_ScPnSh_Sw_HidDiscRV.h \
GMTK_ScPnSh_Sw_ObsContRV.h \
GMTK_ScPnSh_Sw_ObsDiscRV.h \
GMTK_SpaceManager.h \
GMTK_SwContRV.h \
GMTK_UnityScoreMixture.h \
GMTK_USCPT.h \
GMTK_WeightMatrix.h \
GMTK_ZeroScoreMixture.h \
tieStructures.h \
GMTK_LatticeEdgeCPT.h GMTK_LatticeEdgeCPT.cc \
GMTK_Tie.h GMTK_Tie.cc \
tieSupport.h tieSupport.cc \
GMTK_RVInfo.h GMTK_RVInfo.cc \
GMTK_GammaComponent.h GMTK_GammaComponent.cc \
GMTK_BetaComponent.h GMTK_BetaComponent.cc \
GMTK_Partition.h GMTK_Partition.cc \
GMTK_JT_Partition.h GMTK_JT_Partition.cc \
GMTK_PartitionTables.h GMTK_PartitionTables.cc \
GMTK_PartitionStructures.h GMTK_PartitionStructures.cc \
GMTK_BinaryViterbiFileUtils.h GMTK_BinaryViterbiFileUtils.cc \
GMTK_JunctionTreeMessagePassing.cc \
GMTK_JunctionTreeIslandMessagePassing.cc \
version.h

# GMTK_MissingFeatureScaledDiagGaussian.cc 
# GMTK_MissingFeatureScaledDiagGaussian.h 

libXOPT_a_SOURCES = \
GMTK_DiagGaussianOpt.cc \
GMTK_LinMeanCondDiagGaussianOpt.cc \
GMTK_PackCliqueValueOpt.cc

libXOPT_a_CXXFLAGS = $(AM_CXXFLAGS) $(XOPTFLAGS) 


# include register_user_internal_cfunc_dts.cc and 
#   user_internal_cfunc_dts.cc in the tar ball
EXTRA_DIST = \
m4/gpl.txt \
GMTK_dlopenDeterministicMappings.h \
register_user_internal_cfunc_dts.cc \
user_internal_cfunc_dts.cc \
dlopen_cfunc_dts.cc


bin_PROGRAMS = \
gmtkTime \
gmtkJT \
gmtkEMtrain \
gmtkKernel \
gmtkViterbi \
gmtkTriangulate \
gmtkParmConvert \
gmtkTFmerge \
gmtkDTindex \
gmtkNGramIndex \
gmtkTie \
gmtkModelInfo \
gmtkOnline \
gmtkPrint \
$(dmlp_programs) $(GMTKVIZ) $(OSXBUNDLE)
EXTRA_PROGRAMS = gmtkDMLPtrain gmtkViz gmtkViz.app abstest

gmtkTie_SOURCES = gmtkTie.cc

gmtkJT_SOURCES = gmtkJT.cc

gmtkTime_SOURCES = gmtkTime.cc

gmtkEMtrain_SOURCES = gmtkEMtrain.cc


gmtkDMLPtrain_SOURCES = gmtkDMLPtrain.cc
gmtkDMLPtrain_CPPFLAGS = $(AM_CPPFLAGS) $(VERSIONFLAG)
gmtkDMLPtrain_CXXFLAGS = $(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(BLAS_CFLAGS) $(PTHREAD_CFLAGS) $(ISOCXX11FLAGS)
gmtkDMLPtrain_LDADD = $(LDADD) $(PTHREAD_LIBS)
if BUILD_DMLP
gmtkDMLPtrain_LDADD += $(builddir)/../deepMLP/libDMLP.a
endif
if BUILD_PHIPAC
gmtkDMLPtrain_LDADD += $(builddir)/../deepMLP/libPHiPAC.a
endif
gmtkDMLPtrain_LDADD += $(BLAS_LIBS)

gmtkKernel_SOURCES = gmtkKernel.cc

gmtkViterbi_SOURCES = gmtkViterbi.cc

gmtkPrint_SOURCES = gmtkPrint.cc

gmtkTriangulate_SOURCES = gmtkTriangulate.cc

gmtkParmConvert_SOURCES = gmtkParmConvert.cc

gmtkModelInfo_SOURCES = gmtkModelInfo.cc

gmtkTFmerge_SOURCES = gmtkTFmerge.cc

gmtkDTindex_SOURCES = gmtkDTindex.cc

gmtkNGramIndex_SOURCES = gmtkNGramIndex.cc

gmtkOnline_SOURCES = gmtkOnline.cc

#gmtkUnroll_SOURCES = gmtkUnroll.o  libGMTK.a ../miscSupport/libmiscSupport.a .NOEXPORT
#	-@rm -f $@
#	$(compile.cc.link) -Wall -g -DMAIN  -o $@ gmtkUnroll.o $(LIBS)



gmtkViz_SOURCES = gmtkViz.cc
gmtkViz_CXXFLAGS = $(VERSIONFLAG) $(VIZCXXFLAGS) $(DEBUGFLAGS) $(OPTFLAGS) $(X_CFLAGS)
gmtkViz_LDFLAGS = $(VIZLDFLAGS) $(X_PRE_LIBS) $(X_LIBS) $(X_EXTRA_LIBS) -lX11 $(OSXLDFLAGS)
gmtkViz_app_SOURCES = Info.plist.in wxmac.icns

if MAKE_BUNDLE
gmtkViz.app: gmtkViz.app/Contents/PkgInfo

app_verbose   = $(app_verbose_@AM_V@)
app_verbose_  = $(app_verbose_@AM_DEFAULT_V@)
app_verbose_0 = @echo "  APP\t gmtkViz.app";

gmtkViz.app/Contents/PkgInfo: gmtkViz$(EXEEXT) $(srcdir)/Info.plist.in \
                              $(srcdir)/wxmac.icns
	$(app_verbose)mkdir -p gmtkViz.app/Contents/MacOS
	$(AM_V_at)mkdir -p gmtkViz.app/Contents/Resources
	$(AM_V_at)sed -e "s/IDENTIFIER/gmtkViz/" \
	-e "s/EXECUTABLE/gmtkViz/" \
	-e "s/VERSION/$(PACKAGE_VERSION)/" \
	$(srcdir)/Info.plist.in >gmtkViz.app/Contents/Info.plist
	$(AM_V_at)/bin/echo -n "APPL????" >gmtkViz.app/Contents/PkgInfo
	$(AM_V_at)ln -f gmtkViz$(EXEEXT) gmtkViz.app/Contents/MacOS/gmtkViz
	$(AM_V_at)cp -f $(srcdir)/wxmac.icns gmtkViz.app/Contents/Resources/wxmac.icns
endif

check_PROGRAMS =  \
$(dmlp_check_programs) \
flat2vit \
parserTest \
testPackCliqueValue \
testViterbi \
neghead \
dlinktest \
vitnorm

vitnorm_SOURCES = vitnorm.ll

abstest_SOURCES = abstest.cc
abstest_CPPFLAGS = $(AM_CPPFLAGS) $(VERSIONFLAG)
abstest_CXXFLAGS = $(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(BLAS_CFLAGS) $(PTHREAD_CFLAGS) $(ISOCXX11FLAGS) 
abstest_LDADD = $(LDADD) $(PTHREAD_LIBS)
if BUILD_DMLP
abstest_LDADD += $(builddir)/../deepMLP/libDMLP.a
endif
if BUILD_PHIPAC
abstest_LDADD += $(builddir)/../deepMLP/libPHiPAC.a
endif
abstest_LDADD += $(BLAS_LIBS)

# testMDCPT testMTCPT
#testRngDecisionTree testGMParms testFileParser lex.yy
# testHash testHashTree?


dlinktest_SOURCES = dlinktest.cc

testViterbi_SOURCES = testViterbi.ll

neghead_SOURCES = neghead.cc

parserTest_SOURCES = parserTest.cc

#testMDCPT_SOURCES = GMTK_MDCPT.cc GMTK_MDCPT.h
#testMDCPT_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)

#testMTCPT_SOURCES = GMTK_MTCPT.cc GMTK_MTCPT.h  
#testMTCPT_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)

#testHash_SOURCES = GMTK_Hash.cc 
#testHash_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)

#testHashTree_SOURCES = GMTK_HashTree.cc 
#testHashTree_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)

#testRngDecisionTree_SOURCES = GMTK_RngDecisionTree.cc \
#GMTK_RngDecisionTree.h GMTK_RV.cc GMTK_RV.h GMTK_DiscRV.cc GMTK_DiscRV.h
#testRngDecisionTree_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)

testPackCliqueValue_SOURCES = GMTK_PackCliqueValue.cc  GMTK_PackCliqueValue.h
testPackCliqueValue_CXXFLAGS = -DMAIN $(AM_CXXFLAGS)


#testGMParms_SOURCES = GMTK_GMParms.cc GMTK_GMParms.h
#	echo $(objs) > /tmp/gmparms_depts
#	$(compile.cc.link) -Wall -g -DMAIN  -o $@ GMTK_GMParms.cc `sed s,GMTK_GMParms.o,, < /tmp/gmparms_depts` $(LIBS)
#testGMParms_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)


AM_LFLAGS = -B
#lex_yy_SOURCES = GMTK_FileTokenizer.ll
#lex_yy_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)


#testFileParser_SOURCES = \
#GMTK_FileParser.cc GMTK_FileParser.h GMTK_FileTokenizer.ll
#testFileParser_LFLAGS = -B
#testFileParser_CXXFLAGS = -DMAIN \
#$(DEBUGFLAGS) $(OPTFLAGS) $(EXCXXFLAGS) $(WALL) $(ANSI) $(PEDANTIC)

