/*
This is based on the FisImporter from Fuzzy Lite but is adapted to load in FLS files that are produced by SCILAB
 */

#ifndef FL_FLSIMPORTER_H
#define FL_FLSIMPORTER_H

#include "fl/imex/Importer.h"

#include <utility>
#include <vector>

namespace fl {
    class Term;
    class Variable;
    class InputVariable;
    class Engine;

    /**
      The FisImporter class is an Importer that configures an Engine and its
      components from utilizing the Fuzzy Inference System format for Matlab or
      Octave.

      @author Juan Rada-Vilela, Ph.D.
      @see FisExporter
      @see Importer
      @since 4.0
     */

    class FL_API FlsImporter : public Importer {
    public:
        FlsImporter();
        virtual ~FlsImporter() FL_IOVERRIDE;
        FL_DEFAULT_COPY_AND_MOVE(FlsImporter)

        virtual std::string name() const FL_IOVERRIDE;

        virtual Engine* fromString(const std::string& fcl) const FL_IOVERRIDE;

        virtual FlsImporter* clone() const FL_IOVERRIDE;

    protected:
        virtual void importSystem(const std::string& section, Engine* engine,
                std::string& andMethod, std::string& orMethod,
                std::string& impMethod, std::string& aggMethod,
                std::string& defuzzMethod) const;
        virtual void importInput(const std::string& section, Engine* engine, std::pair<size_t, size_t>& inputOutputNum) const;
        virtual void importOutput(const std::string& section, Engine* engine, std::pair<size_t, size_t>& inputOutputNum) const;
        virtual void importRules(const std::string& section, Engine* engine, std::pair<size_t, size_t>& inputOutputNum) const;
        virtual std::string translateProposition(double code, Variable* variable) const;

        virtual std::string translateTNorm(const std::string& tnorm) const;
        virtual std::string translateSNorm(const std::string& tnorm) const;
        virtual std::string translateDefuzzifier(const std::string& defuzzifier) const;

        virtual Term* parseTerm(const std::string& line, const Engine* engine) const;
        virtual Term* createInstance(const std::string& termClass, const std::string& name,
                const std::vector<std::string>& params, const Engine* engine) const;

        virtual void importVariable(const std::string& section, Engine* engine, Variable* variable) const;
        virtual void importTerms(const std::string& section, const Engine* engine, Variable* variable) const;

        virtual std::pair<double, double> parseRange(const std::string& range) const;
    };
}

#endif  /* FL_FLSIMPORTER_H */

