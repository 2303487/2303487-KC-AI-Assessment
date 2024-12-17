/*
This is based on the FisImporter from Fuzzy Lite but is adapted to load in FLS files that are produced by SCILAB
 */
#include "FlsImporter.h"
#include <fstream>
 
#include "fl/Headers.h"


namespace fl {

    FlsImporter::FlsImporter() { }

    FlsImporter::~FlsImporter() { }

    std::string FlsImporter::name() const {
        return "FlsImporter";
    }

    Engine* FlsImporter::fromString(const std::string& fis) const {
        std::unique_ptr<Engine> engine(new Engine);

        std::istringstream fisReader(fis);
        std::string line;
        std::size_t lineNumber = 0;

        std::vector<std::string> sections;
        while (std::getline(fisReader, line)) {
            ++lineNumber;
            //remove comments
            line = Op::split(line, "//", false).front();
            line = Op::split(line, "#", false).front();
            line = Op::trim(line);
            if (line.empty() or line.at(0) == '%') {
                continue;
            }

            line = Op::findReplace(line, "'", "");

            if ("<DESCRIPTION>" == line.substr(0, std::string("<DESCRIPTION>").size())
                    or "<INPUT>" == line.substr(0, std::string("<INPUT>").size())
                    or "<OUTPUT>" == line.substr(0, std::string("<OUTPUT>").size())
                    or "<RULE>" == line.substr(0, std::string("<RULE>").size())) {
                sections.push_back(line);
            }
            else if ("<REVISION>" == line.substr(0, std::string("<REVISION>").size())
                    or "<revision>" == line.substr(0, std::string("<revision>").size())) {
                continue;   // skip in this event
            }
            else {
                if (not sections.empty()) {
                    sections.at(sections.size() - 1) += "\n" + line;
                } else {
                    std::ostringstream ss;
                    ss << "[import error] line " << lineNumber << " <" << line << "> "
                            "does not belong to any section";
                    throw Exception(ss.str(), FL_AT);
                }
            }
        }
        std::pair<size_t, size_t> inputOutputNum;
        std::string andMethod, orMethod, impMethod, aggMethod, defuzzMethod;
        for (std::size_t i = 0; i < sections.size(); ++i) {
            if ("<DESCRIPTION>" == sections.at(i).substr(0, std::string("<DESCRIPTION>").size()))
                importSystem(sections.at(i), engine.get(),
                    andMethod, orMethod, impMethod, aggMethod, defuzzMethod);
            else if ("<INPUT>" == sections.at(i).substr(0, std::string("<INPUT>").size()))
                importInput(sections.at(i), engine.get(), inputOutputNum);
            else if ("<OUTPUT>" == sections.at(i).substr(0, std::string("<OUTPUT>").size()))
                importOutput(sections.at(i), engine.get(), inputOutputNum);
            else if ("<RULE>" == sections.at(i).substr(0, std::string("<RULE>").size()))
                importRules(sections.at(i), engine.get(), inputOutputNum);
            else throw Exception("[import error] section <"
                    + sections.at(i) + "> not recognized", FL_AT);
        }
        engine->configure(translateTNorm(andMethod), translateSNorm(orMethod),
                translateTNorm(impMethod), translateSNorm(aggMethod),
                translateDefuzzifier(defuzzMethod), General().className());
        return engine.release();
    }

    void FlsImporter::importSystem(const std::string& section, Engine * engine,
            std::string& andMethod, std::string& orMethod,
            std::string& impMethod, std::string& aggMethod,
            std::string& defuzzMethod) const {
        std::istringstream reader(section);
        std::string line;
        std::getline(reader, line); //ignore first line [System]
        while (std::getline(reader, line)) {
            std::vector<std::string> keyValue = Op::split(line, ">");

            std::string key = Op::trim(keyValue.at(0));
            std::string value;
            for (std::size_t i = 1; i < keyValue.size(); ++i) {
                value += keyValue.at(i);
            }
            value = Op::trim(value);
            if (key == "<name") engine->setName(value);
            else if (key == "<TNorm") andMethod = value;
            else if (key == "<SNorm") orMethod = value;
            else if (key == "<ImpMethod") impMethod = value;
            else if (key == "<AggMethod") aggMethod = value;
            else if (key == "<defuzzMethod") defuzzMethod = value;
            else if (key == "<type" or key == "<comment"
                    or key == "<Comp" or key == "<CompPar"
                    or key == "<TNormPar" or key == "<SNormPar") {
                //ignore because are redundant.
            } else throw Exception("[import error] token <" + key + "> not recognized", FL_AT);
        }
    }

    void FlsImporter::importInput(const std::string& section, Engine* engine, std::pair<size_t, size_t>& inputOutputNum) const {
        std::istringstream subReader(section);

        std::vector<std::string> subSections;

        std::string line;
        std::size_t lineNumber = 0;

        std::getline(subReader, line); //ignore first line [Input#]
        while (std::getline(subReader, line)) {
            lineNumber++;
            if ("<name>" == line.substr(0, std::string("<name>").size())) {
                subSections.push_back(line);
            }
            else {
                if (not subSections.empty()) {
                    subSections.at(subSections.size() - 1) += "\n" + line;
                }
                else {
                    std::ostringstream ss;
                    ss << "[import error] line " << lineNumber << " <" << line << "> "
                        "does not belong to any section";
                    throw Exception(ss.str(), FL_AT);
                }
            }
        }

        inputOutputNum.first = subSections.size();

        for (std::size_t i = 0; i < subSections.size(); ++i) {
            InputVariable* newVariable = new InputVariable();
            engine->addInputVariable(newVariable);
            importVariable(subSections.at(i), engine, newVariable);
        }
    }

    void FlsImporter::importOutput(const std::string& section, Engine* engine, std::pair<size_t, size_t>& inputOutputNum) const {
        std::istringstream subReader(section);

        std::vector<std::string> subSections;

        std::string line;
        std::size_t lineNumber = 0;

        std::getline(subReader, line); //ignore first line [Input#]
        while (std::getline(subReader, line)) {
            lineNumber++;
            if ("<name>" == line.substr(0, std::string("<name>").size())) {
                subSections.push_back(line);
            }
            else {
                if (not subSections.empty()) {
                    subSections.at(subSections.size() - 1) += "\n" + line;
                }
                else {
                    std::ostringstream ss;
                    ss << "[import error] line " << lineNumber << " <" << line << "> "
                        "does not belong to any section";
                    throw Exception(ss.str(), FL_AT);
                }
            }
        }

        inputOutputNum.second = subSections.size();

        for (std::size_t i = 0; i < subSections.size(); ++i) {
            OutputVariable* newVariable = new OutputVariable();
            engine->addOutputVariable(newVariable);
            importVariable(subSections.at(i), engine, newVariable);
        }
    }

    void FlsImporter::importRules(const std::string& section, Engine* engine, std::pair<size_t, size_t>& inputOutputNum) const {
        std::istringstream reader(section);
        std::string line;
        std::getline(reader, line); //ignore first line [Rules]

        RuleBlock* ruleblock = new RuleBlock;
        engine->addRuleBlock(ruleblock);

        while (std::getline(reader, line)) {
            std::vector<std::string> components = Op::split(line, " ");
            // Extract weighting - last item
            std::string ruleWeight = components.at(components.size() - 1);
            components.erase(components.begin() + components.size() - 1);

            // Extract connector - second last item
            std::string connector = components.at(components.size() - 1);
            components.erase(components.begin() + components.size() - 1);

            if ((engine->numberOfInputVariables() + engine->numberOfOutputVariables())
                != components.size()) {
                std::ostringstream ss;
                ss << "[syntax error] expected <" << engine->numberOfInputVariables() + engine->numberOfOutputVariables() << ">"
                    " components, but found <" << components.size() << ">"
                    " components in rule <" << line << ">";
                throw Exception(ss.str(), FL_AT);
            }

            std::vector<std::string> inputs;
            std::vector<std::string> outputs;
            for (int i = 0; i < (inputOutputNum.first + inputOutputNum.second); i++) {
                if(i < inputOutputNum.first)
                    inputs.push_back(components.at(i));
                else
                    outputs.push_back(components.at(i));
            } 

            std::vector<std::string> antecedent, consequent;

            for (std::size_t i = 0; i < inputs.size(); ++i) {
                scalar inputCode = Op::toScalar(inputs.at(i));
                if (Op::isEq(inputCode, 0.0, 0.000001)) continue;
                std::ostringstream ss;
                ss << engine->getInputVariable(i)->getName() << " "
                        << Rule::isKeyword() << " "
                        << translateProposition(inputCode, engine->getInputVariable(i));
                antecedent.push_back(ss.str());
            }

            for (std::size_t i = 0; i < outputs.size(); ++i) {
                scalar outputCode = Op::toScalar(outputs.at(i));
                if (Op::isEq(outputCode, 0.0, 0.000001)) continue;
                std::ostringstream ss;
                ss << engine->getOutputVariable(i)->getName() << " "
                        << Rule::isKeyword() << " "
                        << translateProposition(outputCode, engine->getOutputVariable(i));
                consequent.push_back(ss.str());
            }

            std::ostringstream ruleText;

            ruleText << Rule::ifKeyword() << " ";
            for (std::size_t i = 0; i < antecedent.size(); ++i) {
                ruleText << antecedent.at(i);
                if (i + 1 < antecedent.size()) {
                    ruleText << " ";
                    if (connector == "1") ruleText << Rule::andKeyword() << " ";
                    else if (connector == "2") ruleText << Rule::orKeyword() << " ";
                    else throw Exception("[syntax error] connector <"
                            + connector + "> not recognized", FL_AT);
                }
            }

            ruleText << " " << Rule::thenKeyword() << " ";
            for (std::size_t i = 0; i < consequent.size(); ++i) {
                ruleText << consequent.at(i);
                if (i + 1 < consequent.size()) {
                    ruleText << " " << Rule::andKeyword() << " ";
                }
            }

            std::ostringstream ss;
            for (std::size_t i = 0; i < ruleWeight.size(); ++i) {
                ss << ruleWeight.at(i);
            }

            scalar weight = Op::toScalar(ss.str());
            if (not Op::isEq(weight, 1.0, 0.000001))
                ruleText << " " << Rule::withKeyword() << " " << Op::str(weight, 3, 0);
            Rule* rule = new Rule(ruleText.str());
            try {
                rule->load(engine);
            } catch (...) {
                //ignore
            }
            ruleblock->addRule(rule);
        }
    }

    std::string FlsImporter::translateProposition(double code, Variable* variable) const {
        int intPart = (int) std::floor(std::abs(code)) - 1;
        scalar fracPart = std::fmod(std::abs(code), scalar(1.0));
        if (intPart >= static_cast<int> (variable->numberOfTerms())) {
            std::ostringstream ex;
            ex << "[syntax error] the code <" << code << "> refers to a term "
                    "out of range from variable <" << variable->getName() << ">";
            throw Exception(ex.str(), FL_AT);
        }

        bool isAny = intPart < 0;
        std::ostringstream ss;
        if (code < 0) ss << Not().name() << " ";
        if (Op::isEq(fracPart, 0.01, 0.000001)) ss << Seldom().name() << " ";
        else if (Op::isEq(fracPart, 0.05, 0.000001)) ss << Somewhat().name() << " ";
        else if (Op::isEq(fracPart, 0.2, 0.000001)) ss << Very().name() << " ";
        else if (Op::isEq(fracPart, 0.3, 0.000001)) ss << Extremely().name() << " ";
        else if (Op::isEq(fracPart, 0.4, 0.000001)) ss << Very().name() << " " << Very().name() << " ";
        else if (Op::isEq(fracPart, 0.99, 0.000001)) ss << Any().name() << " ";
        else if (not Op::isEq(fracPart, 0.0, 0.000001))
            throw Exception("[syntax error] no hedge defined in FIS format for <"
                + Op::str(fracPart, 3, 0) + ">", FL_AT);
        if (not isAny) {
            ss << variable->getTerm(intPart)->getName();
        }
        return ss.str();
    }

    std::string FlsImporter::translateTNorm(const std::string& name) const {
        if (name.empty()) return "";
        if (name == "min") return Minimum().className();
        if (name == "prod") return AlgebraicProduct().className();
        if (name == "bounded_difference") return BoundedDifference().className();
        if (name == "drastic_product") return DrasticProduct().className();
        if (name == "einstein_product") return EinsteinProduct().className();
        if (name == "hamacher_product") return HamacherProduct().className();
        if (name == "nilpotent_minimum") return NilpotentMinimum().className();
        return name;
    }

    std::string FlsImporter::translateSNorm(const std::string& name) const {
        if (name.empty()) return "";
        if (name == "max") return Maximum().className();
        if (name == "probor") return AlgebraicSum().className();
        if (name == "bounded_sum") return BoundedSum().className();
        if (name == "normalized_sum") return NormalizedSum().className();
        if (name == "drastic_sum") return DrasticSum().className();
        if (name == "einstein_sum") return EinsteinSum().className();
        if (name == "hamacher_sum") return HamacherSum().className();
        if (name == "nilpotent_maximum") return NilpotentMaximum().className();
        if (name == "sum") return UnboundedSum().className();
        return name;
    }

    std::string FlsImporter::translateDefuzzifier(const std::string& name) const {
        if (name.empty()) return "";
        if (name == "centroide") return Centroid().className();
        if (name == "bisector") return Bisector().className();
        if (name == "lom") return LargestOfMaximum().className();
        if (name == "mom") return MeanOfMaximum().className();
        if (name == "som") return SmallestOfMaximum().className();
        if (name == "wtaver") return WeightedAverage().className();
        if (name == "wtsum") return WeightedSum().className();
        return name;
    }

    std::pair<double, double> FlsImporter::parseRange(const std::string& range) const {
        std::vector<std::string> parts = Op::split(range, " ");
        if (parts.size() != 2)
            throw Exception("[syntax error] expected range in format 'begin end',"
                " but found incorrect number of parts", FL_AT);
        std::string begin = parts.at(0), end = parts.at(1);
        std::pair<scalar, scalar> result;
        result.first = Op::toScalar(begin);
        result.second = Op::toScalar(end);
        return result;
    }

    Term * FlsImporter::parseTerm(const std::string& fis, const Engine* engine) const {
        std::ostringstream ss;
        for (std::size_t i = 0; i < fis.size(); ++i) {
            if (not (fis.at(i) == '[' or fis.at(i) == ']')) {
                ss << fis.at(i);
            }
        }
        std::string line = ss.str();

        std::vector<std::string> nameTerm = Op::split(line, ":");
        if (nameTerm.size() != 2) {
            throw Exception("[syntax error] expected term in format 'name':'class',[params], "
                    "but found <" + line + ">", FL_AT);
        }
        std::vector<std::string> termParams = Op::split(nameTerm.at(1), ",");
        if (termParams.size() != 2) {
            throw Exception("[syntax error] expected term in format 'name':'class',[params], "
                    "but found " + line, FL_AT);
        }

        std::vector<std::string> parameters = Op::split(termParams.at(1), " ");
        for (std::size_t i = 0; i < parameters.size(); ++i) {
            parameters.at(i) = Op::trim(parameters.at(i));
        }
        return createInstance(
                Op::trim(termParams.at(0)),
                Op::trim(nameTerm.at(0)),
                parameters, engine);
    }

    Term* FlsImporter::createInstance(const std::string& mClass,
            const std::string& name, const std::vector<std::string>& params,
            const Engine* engine) const {
        std::map<std::string, std::string> mapping;
        mapping["binarymf"] = Binary().className();
        mapping["concavemf"] = Concave().className();
        mapping["constant"] = Constant().className();
        mapping["cosinemf"] = Cosine().className();
        mapping["discretemf"] = Discrete().className();
        mapping["function"] = Function().className();
        mapping["gbellmf"] = Bell().className();
        mapping["gaussmf"] = Gaussian().className();
        mapping["gauss2mf"] = GaussianProduct().className();
        mapping["linear"] = Linear().className();
        mapping["pimf"] = PiShape().className();
        mapping["rampmf"] = Ramp().className();
        mapping["rectmf"] = Rectangle().className();
        mapping["smf"] = SShape().className();
        mapping["sigmf"] = Sigmoid().className();
        mapping["dsigmf"] = SigmoidDifference().className();
        mapping["psigmf"] = SigmoidProduct().className();
        mapping["spikemf"] = Spike().className();
        mapping["trapmf"] = Trapezoid().className();
        mapping["trimf"] = Triangle().className();
        mapping["zmf"] = ZShape().className();

        std::vector<std::string> sortedParams = params;

        if (mClass == "gbellmf" and params.size() >= 3) {
            sortedParams.at(0) = params.at(2);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(1);
        } else if (mClass == "gaussmf" and params.size() >= 2) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
        } else if (mClass == "gauss2mf" and params.size() >= 4) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(3);
            sortedParams.at(3) = params.at(2);
        } else if (mClass == "sigmf" and params.size() >= 2) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
        } else if (mClass == "dsigmf" and params.size() >= 4) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(2);
            sortedParams.at(3) = params.at(3);
        } else if (mClass == "psigmf" and params.size() >= 4) {
            sortedParams.at(0) = params.at(1);
            sortedParams.at(1) = params.at(0);
            sortedParams.at(2) = params.at(2);
            sortedParams.at(3) = params.at(3);
        }

        std::string flClass;
        std::map<std::string, std::string>::const_iterator it = mapping.find(mClass);
        if (it != mapping.end()) flClass = it->second;
        else flClass = mClass;

        FL_unique_ptr<Term> term;
        term.reset(FactoryManager::instance()->term()->constructObject(flClass));
        term->updateReference(engine);
        term->setName(Op::validName(name));
        std::string separator;
        if (not dynamic_cast<Function*> (term.get())) {
            separator = " ";
        }
        term->configure(Op::join(sortedParams, separator));
        return term.release();
    }

    void FlsImporter::importVariable(const std::string& section, Engine* engine, Variable* variable) const
    {
        std::istringstream termReader(section);
        std::string line;
        std::vector<std::string> termSections;
        while (std::getline(termReader, line)) {
            if ("<name>" == line.substr(0, std::string("<name>").size())
                or "<range>" == line.substr(0, std::string("<range>").size()))
            {
                continue;
            }
            if ("<mf_name>" == line.substr(0, std::string("<mf_name>").size()))
            {
                termSections.push_back(line);
            }
            else {
                if (not termSections.empty()) {
                    termSections.at(termSections.size() - 1) += "\n" + line;
                }
            }
        }

        std::istringstream reader(section);

        while (std::getline(reader, line)) {
            std::vector<std::string> keyValue = Op::split(line, ">");
            if (keyValue.size() != 2)
                throw Exception("[syntax error] expected a property of type "
                    "'key=value', but found <" + line + ">", FL_AT);
            std::string key = Op::trim(keyValue.at(0));
            std::string value = Op::trim(keyValue.at(1));

            if (key == "<name") {
                variable->setName(Op::validName(value));
            }
            else if (key == "<range") {
                std::pair<scalar, scalar> minmax = parseRange(value);
                variable->setMinimum(minmax.first);
                variable->setMaximum(minmax.second);
            }
            else if (key == "<mf_name"
                or key == "<mf_type"
                or key == "<mf_par") {
                continue;
            }
            else {
                throw Exception("[import error] token <" + key + "> not recognized", FL_AT);
            }
        }

        for (std::size_t i = 0; i < termSections.size(); ++i) {
            importTerms(termSections.at(i), engine, variable);
        }
    }

    void FlsImporter::importTerms(const std::string& section, const Engine* engine, Variable* input) const
    {
        std::istringstream reader(section);
        std::string line;

        std::string termName;
        std::string termType;
        std::string termRanges;

        while (std::getline(reader, line)) {
            std::vector<std::string> keyValue = Op::split(line, ">");
            if (keyValue.size() != 2)
                throw Exception("[syntax error] expected a property of type "
                    "'key=value', but found <" + line + ">", FL_AT);
            std::string key = Op::trim(keyValue.at(0));
            std::string value = Op::trim(keyValue.at(1));

            if (key == "<mf_name") {
                termName = value;
            }
            else if (key == "<mf_type")
            {
                termType = value;
            }
            else if (key == "<mf_par") {
                termRanges = value;
            }
            else {
                throw Exception("[import error] token <" + key + "> not recognized", FL_AT);
            }
        }

        input->addTerm(createInstance(termType, termName, Op::split(termRanges, " "), engine));
    }

    FlsImporter* FlsImporter::clone() const {
        return new FlsImporter(*this);
    }
}