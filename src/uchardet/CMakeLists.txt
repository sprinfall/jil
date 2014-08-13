set(SRCS
    CharDistribution.cpp
    CharDistribution.h
    CMakeLists.txt
    JpCntx.cpp
    JpCntx.h
    LangBulgarianModel.cpp
    LangCyrillicModel.cpp
    LangGreekModel.cpp
    LangHebrewModel.cpp
    LangHungarianModel.cpp
    LangThaiModel.cpp
    Makefile.in
    nsBig5Prober.cpp
    nsBig5Prober.h
    nsCharSetProber.cpp
    nsCharSetProber.h
    nsCodingStateMachine.h
    nsEscCharsetProber.cpp
    nsEscCharsetProber.h
    nsEscSM.cpp
    nsEUCJPProber.cpp
    nsEUCJPProber.h
    nsEUCKRProber.cpp
    nsEUCKRProber.h
    nsEUCTWProber.cpp
    nsEUCTWProber.h
    nsGB2312Prober.cpp
    nsGB2312Prober.h
    nsHebrewProber.cpp
    nsHebrewProber.h
    nsLatin1Prober.cpp
    nsLatin1Prober.h
    nsMBCSGroupProber.cpp
    nsMBCSGroupProber.h
    nsMBCSSM.cpp
    nsPkgInt.h
    nsSBCharSetProber.cpp
    nsSBCharSetProber.h
    nsSBCSGroupProber.cpp
    nsSBCSGroupProber.h
    nsSJISProber.cpp
    nsSJISProber.h
    nsUniversalDetector.cpp
    nsUniversalDetector.h
    nsUTF8Prober.cpp
    nsUTF8Prober.h
    )

#add_definitions()

add_library(uchardet ${SRCS})

#install(TARGETS uchardet DESTINATION .)
