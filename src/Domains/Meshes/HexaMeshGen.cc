#include <fstream>
#include <boost/algorithm/string.hpp>

#include "HexaMeshGen.h"
#include "Output.h"

HexaMeshGen::HexaMeshGen()
    :
      metricConversion_(1.)
{
    
}

HexaMeshGen::HexaMeshGen(int argc, const char *argv[])
    :
      HexaMeshGen()
{
    
    argsList_.readArgs(argc, argv);
    
}

void HexaMeshGen::processBuffer(std::string &buffer, bool removeAllWhitespace)
{
    using namespace boost::algorithm;
    
    // Trim the leading/lagging whitespace

    trim(buffer);

    // Remove all whitespace from buffer if desired (default)
    
    if(removeAllWhitespace)
        erase_all(buffer, " ");
    
    // Check if it is a comment line, if so discard the input
    
    if(buffer[0] == '#')
    {
        
        buffer.clear();
        
    }
    
    // Remove any comments on the line
    
    buffer = buffer.substr(0, buffer.find("#"));
    
}

void HexaMeshGen::readVertices(std::ifstream& inputFile)
{
    using namespace std;

    Point3D tempVertex;
    string buffer;

    // Make sure the vertex list is empty since push_back is used

    vertices_.clear();

    while(!inputFile.eof())
    {
        
        // Get a line from the file and process it

        getline(inputFile, buffer);
        processBuffer(buffer);
        
        if(buffer.empty())
        {

            continue;

        }
        else if(buffer != "{")
        {

            throw ("Expected a \"{\", but received a \"" + buffer + "\".").c_str();

        }

        // Input is good, break

        break;

    }

    // Begin reading the vertices

    while(true)
    {

        getline(inputFile, buffer);
        processBuffer(buffer, false);

        if(buffer.empty())
            continue;

        if(buffer == "}")
            break;


        // Extract the bracketed coordinate

        buffer = buffer.substr(buffer.find_first_of("("), buffer.find_first_of(")") + 1);

        // Begin extracting the vertex coordinates from the buffer

        tempVertex.x = getNextElement(buffer);
        tempVertex.y = getNextElement(buffer);
        tempVertex.z = getNextElement(buffer);
        vertices_.push_back(tempVertex);

        if(buffer != ")")
        {

            throw "Expected a \")\"";

        }

    }

    Output::printToScreen("HexaMeshGen: Successfully initialized domain vertices.");

}


void HexaMeshGen::readResolution(std::ifstream& inputFile)
{
    using namespace std;

    int nI, nJ, nK;
    string buffer;

    while(!inputFile.eof())
    {

        // Get a line from the file and process it

        getline(inputFile, buffer);
        processBuffer(buffer);

        if(buffer.empty())
        {

            continue;

        }
        else if(buffer != "{")
        {

            throw ("Expected a \"{\", but received a \"" + buffer + "\".").c_str();

        }

        // Input is good, break

        break;

    }

    // Begin reading the vertices

    while(true)
    {

        getline(inputFile, buffer);
        processBuffer(buffer, false);

        if(buffer.empty())
            continue;

        if(buffer == "}")
            break;


        // Extract the bracketed coordinate

        buffer = buffer.substr(buffer.find_first_of("("), buffer.find_first_of(")") + 1);

        // Begin extracting the vertex coordinates from the buffer

        nI = int(getNextElement(buffer));
        nJ = int(getNextElement(buffer));
        nK = int(getNextElement(buffer));
        nodes_.allocate(nI, nJ, nK);

        if(buffer != ")")
        {

            throw "Expected a \")\"";

        }

    }

    Output::printToScreen("HexaMeshGen: Successfully allocated mesh nodes.");

}

double HexaMeshGen::getNextElement(std::string &buffer)
{
    using namespace boost::algorithm;

    double element;

    // Ensure the buffer is trimmed

    trim_left_if(buffer, is_any_of("( "));

    // Extract a double element from the string

    element = stod(buffer.substr(0, buffer.find_first_of(" )")));

    // Remove the extracted element from the string

    buffer = buffer.substr(buffer.find_first_of(" )"), buffer.back());

    return element;

}

void HexaMeshGen::readMeshInputFile()
{
    using namespace std;
    
    string buffer;
    ifstream inputFile(argsList_.varsMap_["file"].as<string>().c_str());
    
    if(!inputFile.is_open())
    {
        
        throw "Mesh input file not found.";
        
    }
    
    while(!inputFile.eof())
    {
        
        // Get a line from the buffer and process it
        
        getline(inputFile, buffer);
        processBuffer(buffer);
        
        // Check to see if the buffer is empty
        
        if(buffer.empty())
            continue;
        
        // Check the contents of the buffer, which must be a header. Pass the inputfile to the apropriate method
        
        if(buffer.substr(0, buffer.find("=")) == "MetricConversion")
        {
            
            // Extract the metric conversion floating point number and store
            
            buffer = buffer.substr(buffer.find("=") + 1, buffer.back());
            
            metricConversion_ = stod(buffer);
            
        }
        else if(buffer == "Vertices")
        {
            
            readVertices(inputFile);
            
        }
        else if(buffer == "Resolution")
        {
            
            readResolution(inputFile);
            
        }
        else
        {
            
            throw ("Unrecognized input field header " + buffer + ".").c_str();
            
        }
        
    }
    
}

void HexaMeshGen::writeMeshFile()
{
    using namespace std;

    int i, j, k;

    ofstream fout("mesh.msh");

    fout << "# CAFFE structured mesh file\n\n"
         << "nNodesI=" << nodes_.sizeI() << " nNodesJ=" << nodes_.sizeJ() << " nNodesK=" << nodes_.sizeK() << "\n";

    for(k = 0; k < nodes_.sizeK(); ++k)
    {

        for(j = 0; j < nodes_.sizeJ(); ++j)
        {

            for(i = 0; i < nodes_.sizeI(); ++i)
            {

                fout << nodes_(i, j, k) << " ";

            } // end for i

            fout << "\n";

        } // end for j
    } // end for k

    fout.close();

}

void HexaMeshGen::generateMesh()
{
    using namespace std;

    int i, j, k, upperI(nodes_.sizeI() - 1), upperJ(nodes_.sizeJ() - 1), upperK(nodes_.sizeK() - 1);
    double s;
    Vector3D tmp1, tmp2;

    // Generate surface meshes on the west and east sides

    for(k = 0; k <= upperK; ++k)
    {

        s = double(k)/double(upperK);

        nodes_(0, 0, k) = (vertices_[4] - vertices_[0])*s + vertices_[0];
        nodes_(0, upperJ, k) = (vertices_[7] - vertices_[3])*s + vertices_[3];
        nodes_(upperI, 0, k) = (vertices_[5] - vertices_[1])*s + vertices_[1];
        nodes_(upperI, upperJ, k) = (vertices_[6] - vertices_[2])*s + vertices_[2];

        tmp1 = nodes_(0, upperJ, k) - nodes_(0, 0, k);
        tmp2 = nodes_(upperI, upperJ, k) - nodes_(upperI, 0, k);

        for(j = 0; j <= upperJ; ++j)
        {

            s = double(j)/double(upperJ);

            nodes_(0, j, k) = tmp1*s + nodes_(0, 0, k);
            nodes_(upperI, j, k) = tmp2*s + nodes_(upperI, 0, k);

        } // end for j
    } // end for k

    // Generate the 3D mesh using the opposing surface meshes

    for(k = 0; k <= upperK; ++k)
    {

        for(j = 0; j <= upperJ; ++j)
        {

            tmp1 = nodes_(upperI, j, k) - nodes_(0, j, k);

            for(i = 0; i <= upperI; ++i)
            {

                s = double(i)/double(upperI);

                nodes_(i, j, k) = tmp1*s + nodes_(0, j, k);

            } // end for i
        } // end for j
    } // end for k

    Output::printToScreen("HexaMeshGen: Mesh generation complete.");

}

void HexaMeshGen::generateBoxMesh(double dx, double dy, double dz)
{
    
    int i, j, k;

    for(k = 0; k < nodes_.sizeK(); ++k)
    {
        for(j = 0; j < nodes_.sizeJ(); ++j)
        {
            for(i = 0; i < nodes_.sizeI(); ++i)
            {

                nodes_(i, j, k) = Point3D( dx*double(i)/double(nodes_.sizeI() - 1),
                                           dy*double(j)/double(nodes_.sizeJ() - 1),
                                           dz*double(k)/double(nodes_.sizeK() - 1) );

            } // end for i
        } // end for j
    } // end for k
    
}
