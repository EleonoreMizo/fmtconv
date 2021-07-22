ACLOCAL_AMFLAGS = -I m4

warningflags = \
        -Wall \
        -Wextra \
        -Wshadow \
        -Wunused \
        -Wnull-dereference \
        -Wvla \
        -Wstrict-aliasing \
        -Wuninitialized \
        -Wunused-parameter \
        -Wreorder \
        -Wsign-compare \
        -Wunreachable-code \
        -Wconversion \
        -Wno-sign-conversion \
         $(COMPWARNFLAGS)

if CLG

# Clang
warningflags += \
        -Wshadow-all \
        -Wshorten-64-to-32 \
        -Wint-conversion \
        -Wconditional-uninitialized \
        -Wconstant-conversion \
        -Wunused-private-field \
        -Wbool-conversion \
        -Wextra-semi \
        -Wnullable-to-nonnull-conversion \
        -Wno-unused-private-field \
        -Wno-unused-command-line-argument
# -Wzero-as-null-pointer-constant

else

# GCC
warningflags += \
        -Wredundant-decls \
        -Wno-ignored-attributes \
        -Wno-expansion-to-defined

endif

warnflagscpp =
includeflags = -I$(srcdir)/../../src
commoncflags = $(DEBUGCFLAGS) $(MFLAGS) $(warningflags) $(includeflags) $(STACKREALIGN)
AM_CXXFLAGS  = -std=$(CXXSTD) $(commoncflags) $(warnflagscpp) $(EXTRA_CXXFLAGS)
AM_LDFLAGS   = $(PLUGINLDFLAGS)

lib_LTLIBRARIES = libfmtconv.la

libfmtconv_la_SOURCES =  \
        ../../src/avstp.h \
        ../../src/AvstpWrapper.cpp \
        ../../src/AvstpWrapper.h \
        ../../src/main.cpp \
        ../../src/types.h \
        ../../src/VapourSynth.h \
        ../../src/conc/AioAdd.h \
        ../../src/conc/AioAdd.hpp \
        ../../src/conc/AioMax.h \
        ../../src/conc/AioMax.hpp \
        ../../src/conc/AioSub.h \
        ../../src/conc/AioSub.hpp \
        ../../src/conc/Array.h \
        ../../src/conc/Array.hpp \
        ../../src/conc/AtomicInt.h \
        ../../src/conc/AtomicInt.hpp \
        ../../src/conc/AtomicIntOp.h \
        ../../src/conc/AtomicIntOp.hpp \
        ../../src/conc/AtomicMem.h \
        ../../src/conc/AtomicMem.hpp \
        ../../src/conc/AtomicPtr.h \
        ../../src/conc/AtomicPtr.hpp \
        ../../src/conc/AtomicPtrIntPair.h \
        ../../src/conc/AtomicPtrIntPair.hpp \
        ../../src/conc/CellPool.h \
        ../../src/conc/CellPool.hpp \
        ../../src/conc/def.h \
        ../../src/conc/fnc.h \
        ../../src/conc/fnc.hpp \
        ../../src/conc/Interlocked.h \
        ../../src/conc/Interlocked.hpp \
        ../../src/conc/LockFreeCell.h \
        ../../src/conc/LockFreeQueue.h \
        ../../src/conc/LockFreeQueue.hpp \
        ../../src/conc/LockFreeStack.h \
        ../../src/conc/LockFreeStack.hpp \
        ../../src/conc/ObjFactoryDef.h \
        ../../src/conc/ObjFactoryDef.hpp \
        ../../src/conc/ObjFactoryInterface.h \
        ../../src/conc/ObjFactoryInterface.hpp \
        ../../src/conc/ObjPool.h \
        ../../src/conc/ObjPool.hpp \
        ../../src/ffft/def.h \
        ../../src/ffft/FFTReal.h \
        ../../src/ffft/FFTReal.hpp \
        ../../src/ffft/OscSinCos.h \
        ../../src/ffft/OscSinCos.hpp \
        ../../src/fmtc/Bitdepth.cpp \
        ../../src/fmtc/Bitdepth.h \
        ../../src/fmtc/Convert.cpp \
        ../../src/fmtc/Convert.h \
        ../../src/fmtc/ConvStep.cpp \
        ../../src/fmtc/ConvStep.h \
        ../../src/fmtc/fnc.cpp \
        ../../src/fmtc/fnc.h \
        ../../src/fmtc/Matrix2020CL.cpp \
        ../../src/fmtc/Matrix2020CL.h \
        ../../src/fmtc/Matrix.cpp \
        ../../src/fmtc/Matrix.h \
        ../../src/fmtc/NativeToStack16.cpp \
        ../../src/fmtc/NativeToStack16.h \
        ../../src/fmtc/Primaries.cpp \
        ../../src/fmtc/Primaries.h \
        ../../src/fmtc/Resample.cpp \
        ../../src/fmtc/Resample.h \
        ../../src/fmtc/SplFmtUtl.h \
        ../../src/fmtc/SplFmtUtl.hpp \
        ../../src/fmtc/Stack16ToNative.cpp \
        ../../src/fmtc/Stack16ToNative.h \
        ../../src/fmtc/Transfer.cpp \
        ../../src/fmtc/Transfer.h \
        ../../src/fmtc/version.h \
        ../../src/fmtcl/ArrayMultiType.cpp \
        ../../src/fmtcl/ArrayMultiType.h \
        ../../src/fmtcl/ArrayMultiType.hpp \
        ../../src/fmtcl/BitBltConv.cpp \
        ../../src/fmtcl/BitBltConv.h \
        ../../src/fmtcl/ChromaPlacement.cpp \
        ../../src/fmtcl/ChromaPlacement.h \
        ../../src/fmtcl/CoefArrInt.cpp \
        ../../src/fmtcl/CoefArrInt.h \
        ../../src/fmtcl/CoefArrInt.hpp \
        ../../src/fmtcl/ColorFamily.h \
        ../../src/fmtcl/ColorSpaceH265.h \
        ../../src/fmtcl/ContFirBlackman.cpp \
        ../../src/fmtcl/ContFirBlackman.h \
        ../../src/fmtcl/ContFirBlackmanMinLobe.cpp \
        ../../src/fmtcl/ContFirBlackmanMinLobe.h \
        ../../src/fmtcl/ContFirCubic.cpp \
        ../../src/fmtcl/ContFirCubic.h \
        ../../src/fmtcl/ContFirFromDiscrete.cpp \
        ../../src/fmtcl/ContFirFromDiscrete.h \
        ../../src/fmtcl/ContFirGauss.cpp \
        ../../src/fmtcl/ContFirGauss.h \
        ../../src/fmtcl/ContFirInterface.cpp \
        ../../src/fmtcl/ContFirInterface.h \
        ../../src/fmtcl/ContFirLanczos.cpp \
        ../../src/fmtcl/ContFirLanczos.h \
        ../../src/fmtcl/ContFirLinear.cpp \
        ../../src/fmtcl/ContFirLinear.h \
        ../../src/fmtcl/ContFirRect.cpp \
        ../../src/fmtcl/ContFirRect.h \
        ../../src/fmtcl/ContFirSinc.cpp \
        ../../src/fmtcl/ContFirSinc.h \
        ../../src/fmtcl/ContFirSnh.cpp \
        ../../src/fmtcl/ContFirSnh.h \
        ../../src/fmtcl/ContFirSpline16.cpp \
        ../../src/fmtcl/ContFirSpline16.h \
        ../../src/fmtcl/ContFirSpline36.cpp \
        ../../src/fmtcl/ContFirSpline36.h \
        ../../src/fmtcl/ContFirSpline64.cpp \
        ../../src/fmtcl/ContFirSpline64.h \
        ../../src/fmtcl/ContFirSpline.cpp \
        ../../src/fmtcl/ContFirSpline.h \
        ../../src/fmtcl/DiscreteFirCustom.cpp \
        ../../src/fmtcl/DiscreteFirCustom.h \
        ../../src/fmtcl/DiscreteFirInterface.cpp \
        ../../src/fmtcl/DiscreteFirInterface.h \
        ../../src/fmtcl/Dither.cpp \
        ../../src/fmtcl/Dither.h \
        ../../src/fmtcl/ErrDifBuf.cpp \
        ../../src/fmtcl/ErrDifBuf.h \
        ../../src/fmtcl/ErrDifBuf.hpp \
        ../../src/fmtcl/ErrDifBufFactory.cpp \
        ../../src/fmtcl/ErrDifBufFactory.h \
        ../../src/fmtcl/FilterResize.cpp \
        ../../src/fmtcl/FilterResize.h \
        ../../src/fmtcl/fnc.cpp \
        ../../src/fmtcl/fnc.h \
        ../../src/fmtcl/InterlacingType.h \
        ../../src/fmtcl/KernelData.cpp \
        ../../src/fmtcl/KernelData.h \
        ../../src/fmtcl/Mat3.h \
        ../../src/fmtcl/Mat3.hpp \
        ../../src/fmtcl/Mat4.h \
        ../../src/fmtcl/Mat4.hpp \
        ../../src/fmtcl/Matrix2020CLProc.cpp \
        ../../src/fmtcl/Matrix2020CLProc.h \
        ../../src/fmtcl/Matrix2020CLProc_macro.h \
        ../../src/fmtcl/MatrixProc.cpp \
        ../../src/fmtcl/MatrixProc.h \
        ../../src/fmtcl/MatrixProc_macro.h \
        ../../src/fmtcl/MatrixUtil.cpp \
        ../../src/fmtcl/MatrixUtil.h \
        ../../src/fmtcl/MatrixWrap.h \
        ../../src/fmtcl/MatrixWrap.hpp \
        ../../src/fmtcl/PicFmt.h \
        ../../src/fmtcl/PrimariesPreset.h \
        ../../src/fmtcl/PrimUtil.cpp \
        ../../src/fmtcl/PrimUtil.h \
        ../../src/fmtcl/Proxy.h \
        ../../src/fmtcl/Proxy.hpp \
        ../../src/fmtcl/ProxyRwCpp.h \
        ../../src/fmtcl/ProxyRwCpp.hpp \
        ../../src/fmtcl/ProxyRwSse2.h \
        ../../src/fmtcl/ProxyRwSse2.hpp \
        ../../src/fmtcl/ReadWrapperFlt.h \
        ../../src/fmtcl/ReadWrapperFlt.hpp \
        ../../src/fmtcl/ReadWrapperInt.h \
        ../../src/fmtcl/ReadWrapperInt.hpp \
        ../../src/fmtcl/ResamplePlaneData.h \
        ../../src/fmtcl/ResampleSpecPlane.cpp \
        ../../src/fmtcl/ResampleSpecPlane.h \
        ../../src/fmtcl/ResampleUtil.cpp \
        ../../src/fmtcl/ResampleUtil.h \
        ../../src/fmtcl/ResizeData.cpp \
        ../../src/fmtcl/ResizeData.h \
        ../../src/fmtcl/ResizeData.hpp \
        ../../src/fmtcl/ResizeDataFactory.cpp \
        ../../src/fmtcl/ResizeDataFactory.h \
        ../../src/fmtcl/RgbSystem.h \
        ../../src/fmtcl/RgbSystem.cpp \
        ../../src/fmtcl/Scaler.cpp \
        ../../src/fmtcl/Scaler.h \
        ../../src/fmtcl/ScalerCopy.h \
        ../../src/fmtcl/SplFmt.h \
        ../../src/fmtcl/SplFmt.hpp \
        ../../src/fmtcl/TransCurve.h \
        ../../src/fmtcl/TransLut.cpp \
        ../../src/fmtcl/TransLut.h \
        ../../src/fmtcl/TransOp2084.cpp \
        ../../src/fmtcl/TransOp2084.h \
        ../../src/fmtcl/TransOpAcesCc.cpp \
        ../../src/fmtcl/TransOpAcesCc.h \
        ../../src/fmtcl/TransOpAffine.cpp \
        ../../src/fmtcl/TransOpAffine.h \
        ../../src/fmtcl/TransOpBypass.h \
        ../../src/fmtcl/TransOpCanonLog.cpp \
        ../../src/fmtcl/TransOpCanonLog.h \
        ../../src/fmtcl/TransOpCompose.h \
        ../../src/fmtcl/TransOpCompose.hpp \
        ../../src/fmtcl/TransOpContrast.h \
        ../../src/fmtcl/TransOpContrast.hpp \
        ../../src/fmtcl/TransOpErimm.cpp \
        ../../src/fmtcl/TransOpErimm.h \
        ../../src/fmtcl/TransOpFilmStream.cpp \
        ../../src/fmtcl/TransOpFilmStream.h \
        ../../src/fmtcl/TransOpHlg.cpp \
        ../../src/fmtcl/TransOpHlg.h \
        ../../src/fmtcl/TransOpInterface.h \
        ../../src/fmtcl/TransOpLinPow.cpp \
        ../../src/fmtcl/TransOpLinPow.h \
        ../../src/fmtcl/TransOpLogC.cpp \
        ../../src/fmtcl/TransOpLogC.h \
        ../../src/fmtcl/TransOpLogTrunc.cpp \
        ../../src/fmtcl/TransOpLogTrunc.h \
        ../../src/fmtcl/TransOpPow.cpp \
        ../../src/fmtcl/TransOpPow.h \
        ../../src/fmtcl/TransOpSLog.cpp \
        ../../src/fmtcl/TransOpSLog.h \
        ../../src/fmtcl/TransOpSLog3.cpp \
        ../../src/fmtcl/TransOpSLog3.h \
        ../../src/fmtcl/TransUtil.cpp \
        ../../src/fmtcl/TransUtil.h \
        ../../src/fmtcl/Vec3.h \
        ../../src/fmtcl/Vec3.hpp \
        ../../src/fmtcl/VoidAndCluster.cpp \
        ../../src/fmtcl/VoidAndCluster.h \
        ../../src/fstb/AllocAlign.h \
        ../../src/fstb/AllocAlign.hpp \
        ../../src/fstb/ArrayAlign.h \
        ../../src/fstb/ArrayAlign.hpp \
        ../../src/fstb/CpuId.cpp \
        ../../src/fstb/CpuId.h \
        ../../src/fstb/def.h \
        ../../src/fstb/fnc_fstb.cpp \
        ../../src/fstb/fnc.h \
        ../../src/fstb/fnc.hpp \
        ../../src/fstb/SingleObj.h \
        ../../src/fstb/SingleObj.hpp \
        ../../src/vsutl/CpuOpt.cpp \
        ../../src/vsutl/CpuOpt.h \
        ../../src/vsutl/FilterBase.cpp \
        ../../src/vsutl/FilterBase.h \
        ../../src/vsutl/fnc.cpp \
        ../../src/vsutl/fnc.h \
        ../../src/vsutl/FrameRefSPtr.h \
        ../../src/vsutl/FuncRefSPtr.h \
        ../../src/vsutl/NodeRefSPtr.h \
        ../../src/vsutl/ObjRefSPtr.h \
        ../../src/vsutl/ObjRefSPtr.hpp \
        ../../src/vsutl/PlaneProcCbInterface.cpp \
        ../../src/vsutl/PlaneProcCbInterface.h \
        ../../src/vsutl/PlaneProcessor.cpp \
        ../../src/vsutl/PlaneProcessor.h \
        ../../src/vsutl/PlaneProcMode.h \
        ../../src/vsutl/Redirect.h \
        ../../src/vsutl/Redirect.hpp

libfmtconv_la_LDFLAGS = -no-undefined -avoid-version $(PLUGINLDFLAGS)
libfmtconv_la_LIBADD =
noinst_LTLIBRARIES =


if X86

libsse2_la_SOURCES = \
        ../../src/fstb/ToolsSse2.cpp \
        ../../src/fstb/ToolsSse2.h \
        ../../src/fstb/ToolsSse2.hpp

libsse2_la_CXXFLAGS = $(AM_CXXFLAGS) -msse2
libfmtconv_la_LIBADD += libsse2.la
noinst_LTLIBRARIES += libsse2.la

libavx_la_SOURCES = \
        ../../src/fmtcl/MatrixProc_avx.cpp

libavx_la_CXXFLAGS = $(AM_CXXFLAGS) -mavx
libfmtconv_la_LIBADD += libavx.la
noinst_LTLIBRARIES += libavx.la

libavx2_la_SOURCES = \
        ../../src/fmtcl/BitBltConv_avx2.cpp \
        ../../src/fmtcl/MatrixProc_avx2.cpp \
        ../../src/fmtcl/ProxyRwAvx2.h \
        ../../src/fmtcl/ProxyRwAvx2.hpp \
        ../../src/fmtcl/Scaler_avx2.cpp \
        ../../src/fmtcl/TransLut_avx2.cpp \
        ../../src/fstb/ToolsAvx2.cpp \
        ../../src/fstb/ToolsAvx2.h \
        ../../src/fstb/ToolsAvx2.hpp

libavx2_la_CXXFLAGS = $(AM_CXXFLAGS) -mavx2
libfmtconv_la_LIBADD += libavx2.la
noinst_LTLIBRARIES += libavx2.la

endif
