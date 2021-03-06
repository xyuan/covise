/* This file is part of COVISE.

   You can use it under the terms of the GNU Lesser General Public License
   version 2.1 or later, see lgpl-2.1.txt.

 * License: LGPL 2+ */

// **************************************************************************
//
//			Source File
//
// * Description    : Read Particle data by RECOM
//
// * Class(es)      :
//
// * inherited from :
//
// * Author  : Uwe Woessner
//
// * History : started 25.3.2010
//
// **************************************************************************

#include <cover/coVRPluginSupport.h>
#include "ParticleViewer.h"
#include <osg/Material>
#include <osg/MatrixTransform>
#include <osg/ShapeDrawable>
#include <osg/Shape>
#include <osg/Geode>
#include <osg/CullFace>
#include <osg/Switch>
#include <osg/Sequence>
#include <osg/Geometry>
#include <osg/BlendFunc>
#include <osg/AlphaFunc>
#include <osg/LineWidth>
#include <PluginUtil/coSphere.h>
#include <cover/coVRAnimationManager.h>
#include <cover/coHud.h>
#include <cover/OpenCOVER.h>
#include <cover/coVRConfig.h>
#include <cover/coVRShader.h>
#include <util/byteswap.h>

//! Function to clean up a string for further computation
/*! Remove tabs, multpile whitespaces, comments etc.
 *\param Reference to the string which should be cleared
 *\param Reference to a string which holds all comment characters
 *\return Copy of the cleared input string
 */
std::string
clearString(const std::string &str, std::string comment)
{
    std::string argv;
    std::string::size_type pos;
    argv = str;
    if (argv.length() == 0)
        return (argv);

    // Remove all comments from the string
    for (pos = 0; pos < comment.length(); pos++)
    {
        if (argv.find(comment[pos]) != std::string::npos)
            argv = argv.erase(argv.find(comment[pos]), argv.length());
    }

    // Skip the rest if the whole line was a comment
    if (0 == argv.length())
        return (argv);

    // Replace tabs with a space
    pos = argv.find("\t");
    while (pos != std::string::npos)
    {
        argv = argv.replace(pos, 1, " ");
        pos = argv.find("\t");
    }

    // Replace multiple spaces with just one
    pos = argv.find("  ");
    while (pos != std::string::npos)
    {
        argv = argv.replace(pos, 2, " ");
        pos = argv.find("  ");
    }

    // Erase first char if it is a space
    if ((argv[0] == ' ') && (argv.length() > 0))
        argv = argv.erase(0, 1);

    // Erase last char if it is a space
    pos = argv.length();
    if (pos-- > 0)
        if (argv[pos] == ' ')
            argv = argv.erase(pos, 1);

    // Clear String if it is just the EOL char
    if (argv[0] == '\0')
        argv.clear();

    return (argv);
}

int
split(std::vector<std::string> &fields, const std::string &str, std::string fieldseparator /* = " " */)
{
    fields.clear();
    std::string worker = clearString(str, "");

    // Replace tabs with a space
    std::string sub;
    std::string::size_type pos;
    pos = worker.find(fieldseparator);
    while (pos != std::string::npos)
    {
        sub = worker.substr(0, pos);
        worker = worker.erase(0, pos + 1);
        fields.push_back(sub);
        pos = worker.find(fieldseparator);
    }
    fields.push_back(worker);
    return (fields.size());
}

//! Template function for string conversion
/*!
 *\param Reference to output value
 *\param Reference to input string
 *\return 0 on success, 1 on failure
 */
template <typename T>
unsigned int
strConvert(T &param, const std::string &str, bool expcast = 0)
{
    // Touch it to prevent warning message, is only used in specialised routines
    expcast = expcast;
    std::stringstream sstr(str);
    sstr >> param;
    if (sstr.width() != 0)
        return (1);

    return (0);
}

struct particleData
{
    float x, y, z;
    float xv, yv, zv;
    float val[100];
};

#define LINE_LEN 1000
using namespace osg;
TimeStepData::TimeStepData(int numParticles, unsigned int nf, unsigned int ni)
    : values(0)
    , Ivalues(0)
{
    numFloats = nf;
    numInts = ni;
    if (nf)
    {
        values = new float *[numFloats];
        for (unsigned int i = 0; i < numFloats; i++)
        {
            values[i] = new float[numParticles];
        }
    }
    if (ni)
    {
        Ivalues = new int64_t *[numInts];
        for (unsigned int i = 0; i < numInts; i++)
        {
            Ivalues[i] = new int64_t[numParticles];
        }
    }
}

TimeStepData::~TimeStepData()
{
    for (unsigned int i = 0; i < numFloats; i++)
    {
        delete[] values[i];
    }
    for (unsigned int i = 0; i < numInts; i++)
    {
        delete[] Ivalues[i];
    }
    delete[] values;
}

extern ParticleViewer *plugin;

int Particles::read64(uint64_t &val)
{
    int res = fread(&val, sizeof(uint64_t), 1, fp);
    if (doSwap)
    {
        byteSwap(val);
    }
    return res;
}
int Particles::read32(int &val)
{
    int res = fread(&val, sizeof(int), 1, fp);
    if (doSwap)
    {
        byteSwap(val);
    }
    return res;
}

Particles::Particles(std::string filename, osg::Group *parent, int maxParticles)
{
    (void)maxParticles;
    numHiddenVars = 6;
    fileName = filename;
    numTimesteps = 0;
    interval = 20;
    numInts = 0;
    numFloats = 0;
    int minNumber;
    ParticleMode = M_PARTICLES;
    lineColor.set(0, 0, 1, 1);
    imwfFormat = false;
    std::string suffix;
    if (filename.length() > 6 && strcmp(filename.c_str() + filename.length() - 5, "coord") == 0)
    {
        imwfFormat = true;
        suffix = "coord";
    }
    if (filename.length() > 6 && strcmp(filename.c_str() + filename.length() - 5, "chkpt") == 0)
    {
        imwfFormat = true;
        suffix = "chkpt";
    }
    if (filename.length() > 6 && strcmp(filename.c_str() + filename.length() - 5, "crist") == 0)
    {
        imwfFormat = true;
        suffix = "crist";
    }
    size_t pos = filename.find("line");
    std::string filenamebegin = filename.substr(0, pos);
    if (pos != string::npos)
    {
        ParticleMode = M_LINES;
        if (filename[pos + 4] == '_')
        {
            std::string tmpShaderName = filename.substr(pos + 5);
            pos = tmpShaderName.find("_");
            if (pos != string::npos)
            {
                shaderName = tmpShaderName.substr(0, pos);
                do
                {
                    tmpShaderName = tmpShaderName.substr(pos + 1);
                    pos = tmpShaderName.find("=");
                    if (pos != string::npos)
                    {
                        std::string param = tmpShaderName.substr(0, pos);
                        tmpShaderName = tmpShaderName.substr(pos + 1);
                        pos = tmpShaderName.find("_");
                        if (pos != string::npos)
                        {
                            std::string paramValue = tmpShaderName.substr(0, pos);
                            if (param == "R")
                            {
                                float val;
                                sscanf(paramValue.c_str(), "%f", &val);
                                lineColor[0] = val;
                            }
                            else if (param == "G")
                            {
                                float val;
                                sscanf(paramValue.c_str(), "%f", &val);
                                lineColor[1] = val;
                            }
                            else if (param == "B")
                            {
                                float val;
                                sscanf(paramValue.c_str(), "%f", &val);
                                lineColor[2] = val;
                            }
                            shaderParams[param] = paramValue;
                        }
                    }
                } while (pos != string::npos);
            }
        }
    }

    //switchNode=new osg::Sequence();
    switchNode = new osg::Switch();
    if (shaderName.length() > 1)
    {
        cerr << "trying shader " << shaderName << endl;
    }
    switchNode->setName(filename);
    shader = coVRShaderList::instance()->get(shaderName, &shaderParams);
    if (parent)
    {
        parent->addChild(switchNode);
    }
    else
    {
        cover->getObjectsRoot()->addChild(switchNode);
    }

    size_t found = filenamebegin.rfind('.');
    if (found != string::npos)
    {
        if (imwfFormat)
        {
            found -= 6;
        }

        sscanf(filenamebegin.c_str() + found + 1, "%d", &minNumber);
        std::string filebeg = filenamebegin.substr(0, found + 1);
        numTimesteps = 0;
        while (true)
        {
            char *tmpFileName = new char[found + 200];
            if (imwfFormat)
            {
                sprintf(tmpFileName, "%s%05d.%s", filebeg.c_str(), minNumber + numTimesteps, suffix.c_str());
            }
            else
            {
                sprintf(tmpFileName, "%s%4d.particle", filebeg.c_str(), minNumber + numTimesteps);
            }
            FILE *fp = fopen(tmpFileName, "r");
            delete[] tmpFileName;
            if (fp)
            {
                numTimesteps++;
                fclose(fp);
            }
            else
            {
                break;
            }
        }
        if (numTimesteps > 0)
        {
            timesteps = new TimeStepData *[numTimesteps];
            for (int i = 0; i < numTimesteps; i++)
            {
                char *tmpFileName = new char[found + 200];
                if (imwfFormat)
                {
                    sprintf(tmpFileName, "%s%05d.%s", filebeg.c_str(), minNumber + i, suffix.c_str());
                    if (readIMWFFile(tmpFileName, i) > 0)
                    {
                    }
                    else
                    {
                        cerr << "could not open" << tmpFileName << endl;
                    }
                }
                else
                {
                    sprintf(tmpFileName, "%s%4d.particle", filebeg.c_str(), minNumber + i);
                    if (readFile(tmpFileName, i) > 0)
                    {
                    }
                    else
                    {
                        cerr << "could not open" << tmpFileName << endl;
                    }
                }
                delete[] tmpFileName;
            }
            if (!imwfFormat)
            {
                summUpTimesteps(interval);
                updateColors();
            }
        }
    }

    if (numTimesteps == 0 && !imwfFormat) // we did not find a number of files it might be a binary file
    {
        fp = fopen(filename.c_str(), "rb");
        if (fp)
        {
            uint64_t timestepNumber;
            int blockstart;
            int blockend;
            //numVars=2;
            numFloats = 2;
            numInts = 0;
            fread(&blockstart, sizeof(int), 1, fp);
            char header[201];
            char *tmp = (char *)&blockstart;
            if (tmp[0] == 'R' && tmp[1] == 'E' && tmp[2] == 'C' && tmp[3] == 'O')
            {
                fseek(fp, 0, SEEK_SET);
                fread(&header, 201, 1, fp);
                //RECOM VERSION 0.1 X Y Z VX VY VZ TT D O2 INL PARTN                                            RECOM
                //RECOM R R R R R R R R R I I   								RECOM
                //RECOM Min(float) Min Min Min...   								RECOM
                //RECOM Max(float) Max ...   								RECOM
                //RECOM RadiusScale(float) RadiusScale ...   								RECOM
                //RECOM Interval(int) NumTimeSteps(int)  								RECOM
                if (strncmp(header, "RECOM VERSION 0.2", 17) == 0)
                {
                    char header2[201];
                    fread(&header2, 201, 1, fp);
                    char *buf = header2;
                    buf += 6;
                    numFloats = 0;
                    while (*buf == 'R' || *buf == 'I')
                    {
                        if (*buf == 'R')
                        {
                            variableTypes.push_back(T_FLOAT);
                            if (numFloats >= 6)
                            {
                                variableIndex.push_back(numFloats - 6);
                            }
                            numFloats++;
                        }
                        else
                        {
                            variableTypes.push_back(T_INT);
                            variableIndex.push_back(numInts);
                            numInts++;
                        }
                        //numVars++;
                        buf += 2;
                    }
                    //numVars -= 6 ; // Pos and Velocity
                    numFloats -= 6; // Pos and Velocity
                    buf = header;
                    buf += 33;
                    unsigned int numVars = numFloats + numInts;
                    for (unsigned int i = 0; i < numVars; i++)
                    {
                        char *buf2 = buf + 1;
                        while (*buf2 != ' ' && *buf2 != '\0')
                        {
                            buf2++;
                        }
                        if (*buf2 != '\0')
                        {
                            *buf2 = '\0';
                            variableNames.push_back(std::string(buf));
                        }
                        else
                        {
                            fprintf(stderr, "Wrong header %s\n", header);
                            break;
                        }
                        buf = buf2 + 1;
                    }
                    std::vector<std::string> id_fields_;
                    float tmpf;

                    fread(&header2, 201, 1, fp);
                    split(id_fields_, header2, " ");
                    // Remove HEADER start and end ("RECOM")
                    for (std::vector<std::string>::iterator it = id_fields_.begin(); it != id_fields_.end();)
                    {
                        if ((*it) == "RECOM")
                            it = id_fields_.erase(it);
                        else
                            ++it;
                    }
                    for (unsigned int i = 6; i < numVars + 6; i++)
                    {
                        strConvert(tmpf, id_fields_[i], false);
                        variableMin.push_back(tmpf);
                    }

                    fread(&header2, 201, 1, fp);
                    split(id_fields_, header2, " ");
                    // Remove HEADER start and end ("RECOM")
                    for (std::vector<std::string>::iterator it = id_fields_.begin(); it != id_fields_.end();)
                    {
                        if ((*it) == "RECOM")
                            it = id_fields_.erase(it);
                        else
                            ++it;
                    }
                    for (unsigned int i = 6; i < numVars + 6; i++)
                    {
                        strConvert(tmpf, id_fields_[i], false);
                        variableMax.push_back(tmpf);
                    }

                    fread(&header2, 201, 1, fp);
                    split(id_fields_, header2, " ");
                    // Remove HEADER start and end ("RECOM")
                    for (std::vector<std::string>::iterator it = id_fields_.begin(); it != id_fields_.end();)
                    {
                        if ((*it) == "RECOM")
                            it = id_fields_.erase(it);
                        else
                            ++it;
                    }
                    for (unsigned int i = 6; i < numVars + numHiddenVars; i++)
                    {
                        strConvert(tmpf, id_fields_[i], false);
                        variableScale.push_back(tmpf);
                    }

                    // read interval
                    fread(&header2, 201, 1, fp);
                    buf = header2;
                    buf += 6;
                    while (*buf == ' ' && *buf != '\0')
                    {
                        buf++;
                    }
                    sscanf(buf, "%d", &interval);
                }
                else
                {
                    fprintf(stderr, "Unknown file version %s\n", header);
                }

                fread(&blockstart, sizeof(int), 1, fp);
            }
            else
            {
                for (int ctr = 0; ctr < 6; ++ctr)
                    variableTypes.push_back(T_FLOAT);
                numHiddenVars = 6;
                variableNames.push_back(std::string("TT"));
                variableTypes.push_back(T_FLOAT);
                variableIndex.push_back(0);
                variableNames.push_back(std::string("D"));
                variableTypes.push_back(T_FLOAT);
                variableIndex.push_back(1);
            }
            doSwap = false;
            if (blockstart != 8)
            {
                doSwap = true;
                byteSwap(blockstart);
            }
            if (blockstart == 8)
            {
                read64(timestepNumber);
                read32(blockend);
                if (blockstart == blockend)
                {
                    numTimesteps = (int)timestepNumber;
                    timesteps = new TimeStepData *[numTimesteps];
                    //bool failed = false;
                    for (int i = 0; i < numTimesteps; i++)
                    {
                        if (readBinaryTimestep(i) > 0)
                        {
                        }
                        else
                        {
                            //failed = true;
                            numTimesteps = i;
                            break;
                        }
                    }
                    //if(!failed)
                    //{
                    summUpTimesteps(interval);
                    updateColors();
                    //}
                }
            }
            fclose(fp);
        }
    }

    if (numTimesteps > coVRAnimationManager::instance()->getNumTimesteps())
    {
        coVRAnimationManager::instance()->setNumTimesteps(numTimesteps);
    }
}

void Particles::summUpTimesteps(int increment)
{
    for (int i = 1; i < numTimesteps; i++)
    {
        osg::Geode *geode = (osg::Geode *)switchNode->getChild(i);
        for (unsigned int n = 1; n < geode->getNumDrawables(); n++)
        {
            geode->removeDrawables(n);
        }
        int numTimestepsToAdd = i / increment; //every increment timestep, we add some particles from the start
        int index = i;
        for (int n = 0; n < numTimestepsToAdd; n++)
        {
            index -= increment;
            osg::Geode *earlyGeode = (osg::Geode *)switchNode->getChild(index);
            geode->addDrawable(earlyGeode->getDrawable(0));
        }
    }
}

int Particles::getMode()
{
    return ParticleMode;
}

int Particles::readBinaryTimestep(int timestep)
{
    osg::Geode *geode = new osg::Geode();
    int blockstart, blockend;
    char buf[LINE_LEN];

    uint64_t particleNumber;
    read32(blockstart);
    read64(particleNumber);
    read32(blockend);
    if (blockend != blockstart)
    {
        return -1;
    }

    int numParticles = particleNumber;
    sprintf(buf, "reading num=%d timestep=%d", numParticles, timestep);
    if (timestep % 50 == 0)
    {
        OpenCOVER::instance()->hud->setText2(buf);
        OpenCOVER::instance()->hud->redraw();
    }
    timesteps[timestep] = new TimeStepData(numParticles, numFloats, numInts);
    timesteps[timestep]->numParticles = numParticles;
    geode->setName(buf);
    float *xc = new float[numParticles];
    float *yc = new float[numParticles];
    float *zc = new float[numParticles];
    float *xv = new float[numParticles];
    float *yv = new float[numParticles];
    float *zv = new float[numParticles];
    float **values = timesteps[timestep]->values;
    int64_t **Ivalues = timesteps[timestep]->Ivalues;
    timesteps[timestep]->geode = geode;

    struct particleData oneParticle;
    int blockoverhead = 0;
    for (int i = 0; i < numParticles; i++)
    {
        read32(blockstart);
        if (blockstart != (6 + numFloats) * sizeof(float) + (numInts * sizeof(coInt64)) && blockoverhead == 0)
        {
            if (blockstart != 60)
            {
                cerr << "ERROR" << endl;
                cerr << "expected 32 byte particle data but got block with " << blockstart << " bytes " << endl;
                cerr << "while reading particle " << i << " of " << numParticles << " in timestep " << timestep << endl;
            }
            blockoverhead = blockstart - ((6 + numFloats) * sizeof(float) + (numInts * sizeof(coInt64)));
            if (blockoverhead < 0)
                return -1;
        }
        fread(&oneParticle, (6 + numFloats) * sizeof(float) + (numInts * sizeof(coInt64)), 1, fp);
        if (blockoverhead > 0)
        {
            //fseek(fp,blockoverhead,SEEK_CUR); // strange, blockstart sais 60 but it seems to be only 32....
        }
        unsigned int nf = 0;
        unsigned int ni = 0;
        unsigned int n = 0;
        unsigned int numVars = numFloats + numInts;
        for (unsigned int nv = 0; nv < numVars; nv++)
        {
            if (variableTypes[nv + numHiddenVars] == T_FLOAT)
            {
                values[nf][i] = oneParticle.val[n];
                n++;
                nf++;
            }
            else
            {
                Ivalues[ni][i] = *(int64_t *)(&oneParticle.val[n]);
                n += 2;
                ni++;
            }
        }
        if (blockoverhead > 0)
        {
            xc[i] = oneParticle.xv;
            yc[i] = oneParticle.yv;
            zc[i] = oneParticle.zv;
            xv[i] = oneParticle.x;
            yv[i] = oneParticle.y;
            zv[i] = oneParticle.z;
        }
        else
        {
            xc[i] = oneParticle.x;
            yc[i] = oneParticle.y;
            zc[i] = oneParticle.z;
            xv[i] = oneParticle.xv;
            yv[i] = oneParticle.yv;
            zv[i] = oneParticle.zv;
        }
        if (doSwap)
        {
            for (unsigned int n = 0; n < numInts; n++)
            {
                byteSwap(Ivalues[n][i]);
            }
            for (unsigned int n = 0; n < numFloats; n++)
            {
                byteSwap(values[n][i]);
            }
            values[1][i] *= 200;
            byteSwap(xc[i]);
            byteSwap(yc[i]);
            byteSwap(zc[i]);
            byteSwap(xv[i]);
            byteSwap(yv[i]);
            byteSwap(zv[i]);
        }
        read32(blockend);
        if (blockend != blockstart)
        {
            return -1;
        }
    }

    //if(iRenderMethod == coSphere::RENDER_METHOD_PARTICLE_CLOUD)
    //   transparent = true;
    osg::StateSet *geoState = geode->getOrCreateStateSet();
    //setDefaultMaterial(geoState, transparent);
    geoState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    osg::BlendFunc *blendFunc = new osg::BlendFunc();
    blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
    geoState->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
    osg::AlphaFunc *alphaFunc = new osg::AlphaFunc();
    alphaFunc->setFunction(osg::AlphaFunc::ALWAYS, 1.0);
    geoState->setAttributeAndModes(alphaFunc, osg::StateAttribute::OFF);

    geode->setStateSet(geoState);

    if (ParticleMode == M_PARTICLES)
    {
        coSphere *sphere = new coSphere();
        sphere->setMaxRadius(1);

        timesteps[timestep]->sphere = sphere;
        //sphere->setColorBinding(colorbinding);
        sphere->setRenderMethod(coSphere::RENDER_METHOD_ARB_POINT_SPRITES);
        float *radii = values[1];
        sphere->setCoords(numParticles, xc, yc, zc, radii);
        float f = plugin->getRadius();
        float *rn = new float[numParticles];
        for (int n = 0; n < numParticles; n++)
        {
            rn[n] = radii[n] * f;
        }
        sphere->updateRadii(rn);
        delete[] rn;

        geode->addDrawable(sphere);
        timesteps[timestep]->lines = NULL;
    }
    else
    {
        osg::Geometry *lines = new osg::Geometry();
        lines->setUseDisplayList(coVRConfig::instance()->useDisplayLists());
        lines->setUseVertexBufferObjects(coVRConfig::instance()->useVBOs());

        // set up geometry
        osg::Vec3Array *vert = new osg::Vec3Array;
        osg::DrawArrays *primitives = new osg::DrawArrays(osg::PrimitiveSet::LINES, 0, numParticles * 2);
        for (int i = 0; i < numParticles; i++)
        {
            vert->push_back(osg::Vec3(xc[i], yc[i], zc[i]));
            vert->push_back(osg::Vec3(xc[i] + (xv[i] / 100), yc[i] + (yv[i] / 100), zc[i] + (zv[i] / 100)));
        }
        lines->setVertexArray(vert);
        lines->addPrimitiveSet(primitives);
        osg::Vec4Array *colArr = new osg::Vec4Array();
        colArr->push_back(lineColor);
        lines->setColorArray(colArr);
        lines->setColorBinding(osg::Geometry::BIND_OVERALL);

        timesteps[timestep]->sphere = NULL;
        timesteps[timestep]->lines = lines;
        timesteps[timestep]->colors = colArr;
        geode->addDrawable(lines);

        if (shader)
            shader->apply(geode);
        osg::LineWidth *lineWidth = new osg::LineWidth(6);
        geoState->setAttributeAndModes(lineWidth, osg::StateAttribute::ON);
    }
    switchNode->addChild(geode);
    switchNode->setSingleChildOn(timestep);

    delete[] xc;
    delete[] yc;
    delete[] zc;
    delete[] xv;
    delete[] yv;
    delete[] zv;
    return numParticles;
}

int Particles::readFile(char *fn, int timestep)
{
    FILE *fp = fopen(fn, "r");
    if (fp)
    {

        int numParticles = 0;
        bool binary = false;
        char buf[LINE_LEN];
        osg::Geode *geode = new osg::Geode();
        // is it binary or ascii?
        // read binary num particles
        /* long long particleNumber;
      int blockstart;
      int blockend;
      fread(&blockstart,sizeof(int),1,fp);
      fread(&particleNumber,sizeof(long long),1,fp);
      fread(&blockend,sizeof(int),1,fp);
      if(blockstart!=blockend)
         binary=false;*/
        if (!binary)
        {
            fseek(fp, 0, SEEK_SET);

            while (!feof(fp))
            {
                fgets(buf, LINE_LEN, fp);
                numParticles++;
            }
            fseek(fp, 0, SEEK_SET);
        }
        if (timestep % 10 == 0)
        {
            sprintf(buf, "reading %s, num=%d timestep=%d", fn, numParticles, timestep);
            OpenCOVER::instance()->hud->setText2(buf);
            OpenCOVER::instance()->hud->redraw();
        }
        timesteps[timestep] = new TimeStepData(numParticles, numFloats, numInts);
        timesteps[timestep]->numParticles = numParticles;
        geode->setName(buf);
        float *xc = new float[numParticles];
        float *yc = new float[numParticles];
        float *zc = new float[numParticles];
        float *xv = new float[numParticles];
        float *yv = new float[numParticles];
        float *zv = new float[numParticles];
        float **values = timesteps[timestep]->values;
        timesteps[timestep]->geode = geode;

        int n = 0;
        struct particleData oneParticle;

        if (binary)
        {
            for (int i = 0; i < numParticles; i++)
            {
                int blockstart, blockend;
                fread(&blockstart, sizeof(int), 1, fp);
                fread(&oneParticle, (6 + numFloats) * sizeof(float) + numInts * sizeof(coInt64), 1, fp);
                fread(&blockend, sizeof(int), 1, fp);
                values[0][i] = oneParticle.val[0];
                values[1][i] = oneParticle.val[1] * 200;
                xc[i] = oneParticle.x;
                yc[i] = oneParticle.y;
                zc[i] = oneParticle.z;
                xv[i] = oneParticle.xv;
                yv[i] = oneParticle.yv;
                zv[i] = oneParticle.zv;
            }
        }
        else
        {
            //float radius = plugin->getRadius();
            while (!feof(fp))
            {
                if (n > numParticles)
                {
                    cerr << "oops read more than last time..." << endl;
                    break;
                }
                fgets(buf, LINE_LEN, fp);
                values[0][n] = 200.0;
                values[1][n] = 0.001;
                sscanf(buf, "%f %f %f %f %f %f %f %f", xc + n, yc + n, zc + n, xv + n, yv + n, zv + n, values[0] + n, values[1] + n);
                values[1][n] *= 200;
                n++;
            }
        }
        fclose(fp);

        //if(iRenderMethod == coSphere::RENDER_METHOD_PARTICLE_CLOUD)
        //   transparent = true;
        osg::StateSet *geoState = geode->getOrCreateStateSet();
        //setDefaultMaterial(geoState, transparent);
        geoState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        osg::BlendFunc *blendFunc = new osg::BlendFunc();
        blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
        geoState->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
        osg::AlphaFunc *alphaFunc = new osg::AlphaFunc();
        alphaFunc->setFunction(osg::AlphaFunc::ALWAYS, 1.0);
        geoState->setAttributeAndModes(alphaFunc, osg::StateAttribute::OFF);

        geode->setStateSet(geoState);

        coSphere *sphere = new coSphere();

        sphere->setMaxRadius(1);

        timesteps[timestep]->sphere = sphere;
        //sphere->setColorBinding(colorbinding);
        sphere->setRenderMethod(coSphere::RENDER_METHOD_ARB_POINT_SPRITES);
        sphere->setCoords(numParticles, xc, yc, zc, values[1]);

        geode->addDrawable(sphere);
        switchNode->addChild(geode, true);

        delete[] xc;
        delete[] yc;
        delete[] zc;
        delete[] xv;
        delete[] yv;
        delete[] zv;
        return numParticles;
    }
    else
    {
        return -1;
    }
    return 0;
}

int Particles::readIMWFFile(char *fn, int timestep)
{
    FILE *fp = fopen(fn, "r");
    if (fp)
    {

        int numParticles = 0;
        bool binary = false;
        char buf[LINE_LEN];
        osg::Geode *geode = new osg::Geode();
        int version = 0;
        while (!feof(fp))
        {
            fgets(buf, LINE_LEN, fp);
            if (buf[0] != '#')
            {
                numParticles++;
            }
            else
            {
                if (strncmp(buf, "#C number type mass x y z vx vy vz crist", 40) == 0)
                {
                    version = 1;
                }
                if (strncmp(buf, "#C number type mass x y z vx vy vz Epot eam_rho", 47) == 0)
                {
                    version = 2;
                }
            }
        }
        fseek(fp, 0, SEEK_SET);

        if (timestep % 10 == 0)
        {
            sprintf(buf, "reading %s, num=%d timestep=%d", fn, numParticles, timestep);
            OpenCOVER::instance()->hud->setText2(buf);
            OpenCOVER::instance()->hud->redraw();
            switchNode->setAllChildrenOff();
        }

        //42 1 1.572263 1.326834 62.641356 14.000000 14.000000 0.000000 0.000000
        numFloats = 4;
        numInts = 2;

        if (version == 0)
        {
            numHiddenVars = 3;
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
        }
        else
        {
            numHiddenVars = 6;
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
            variableTypes.push_back(T_FLOAT);
        }

        if (version == 0)
        {
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(0);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("N_FE"));
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(1);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("N_CU"));
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(2);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("N_NI"));
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(3);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
        }
        if (version == 1)
        {

            numFloats = 8;
            numInts = 2;
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(0);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("Mass"));
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(1);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("crist"));
        }
        if (version == 2)
        {
            numFloats = 9;
            numInts = 2;
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(0);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("Mass"));
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(1);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("Epot"));
            variableTypes.push_back(T_FLOAT);
            variableIndex.push_back(2);
            variableScale.push_back(1.0);
            variableMin.push_back(1.0);
            variableMax.push_back(18.0);
            variableNames.push_back(std::string("eam_rho"));
        }

        variableNames.push_back(std::string("Number"));
        variableTypes.push_back(T_INT);
        variableIndex.push_back(0);
        variableScale.push_back(1.0);
        variableMin.push_back(1.0);
        variableMax.push_back(18.0);
        variableNames.push_back(std::string("TYPE"));
        variableTypes.push_back(T_INT);
        variableIndex.push_back(1);
        variableScale.push_back(1.0);
        variableMin.push_back(1.0);
        variableMax.push_back(18.0);

        timesteps[timestep] = new TimeStepData(numParticles, numFloats, numInts);
        timesteps[timestep]->numParticles = numParticles;
        geode->setName(buf);
        float *xc = new float[numParticles];
        float *yc = new float[numParticles];
        float *zc = new float[numParticles];
        float **values = timesteps[timestep]->values;
        int64_t **Ivalues = timesteps[timestep]->Ivalues;
        timesteps[timestep]->geode = geode;

        int n = 0;
        struct particleData oneParticle;

        //float radius = plugin->getRadius();
        if (version == 0)
        {
            while (!feof(fp))
            {
                if (n > numParticles)
                {
                    cerr << "oops read more than last time..." << endl;
                    break;
                }
                fgets(buf, LINE_LEN, fp);
                if (buf[0] != '#')
                {
                    int num, type;
                    sscanf(buf, "%d %d %f %f %f %f %f %f %f", &num, &type, xc + n, yc + n, zc + n, values[0] + n, values[1] + n, values[2] + n, values[3] + n);
                    Ivalues[0][n] = num;
                    Ivalues[1][n] = type;
                    n++;
                }
            }
        }
        if (version == 1)
        {
            while (!feof(fp))
            {
                if (n > numParticles)
                {
                    cerr << "oops read more than last time..." << endl;
                    break;
                }
                fgets(buf, LINE_LEN, fp);
                if (buf[0] != '#')
                {
                    int num, type;
                    float tmpf;
                    sscanf(buf, "%d %d %f %f %f %f %f %f %f %f", &num, &type, values[0] + n, xc + n, yc + n, zc + n, &tmpf, &tmpf, &tmpf, values[1] + n);
                    Ivalues[0][n] = num;
                    Ivalues[1][n] = type;
                    n++;
                }
            }
        }
        if (version == 2)
        {
            while (!feof(fp))
            {
                if (n > numParticles)
                {
                    cerr << "oops read more than last time..." << endl;
                    break;
                }
                fgets(buf, LINE_LEN, fp);
                if (buf[0] != '#')
                {
                    int num, type;
                    float tmpf;
                    sscanf(buf, "%d %d %f %f %f %f %f %f %f %f %f", &num, &type, values[0] + n, xc + n, yc + n, zc + n, &tmpf, &tmpf, &tmpf, values[1] + n, values[2] + n);
                    Ivalues[0][n] = num;
                    Ivalues[1][n] = type;
                    n++;
                }
            }
        }
        fclose(fp);

        //if(iRenderMethod == coSphere::RENDER_METHOD_PARTICLE_CLOUD)
        //   transparent = true;
        osg::StateSet *geoState = geode->getOrCreateStateSet();
        //setDefaultMaterial(geoState, transparent);
        geoState->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

        osg::BlendFunc *blendFunc = new osg::BlendFunc();
        blendFunc->setFunction(osg::BlendFunc::SRC_ALPHA, osg::BlendFunc::ONE_MINUS_SRC_ALPHA);
        geoState->setAttributeAndModes(blendFunc, osg::StateAttribute::ON);
        osg::AlphaFunc *alphaFunc = new osg::AlphaFunc();
        alphaFunc->setFunction(osg::AlphaFunc::ALWAYS, 1.0);
        geoState->setAttributeAndModes(alphaFunc, osg::StateAttribute::OFF);

        geode->setStateSet(geoState);

        coSphere *sphere = new coSphere();

        sphere->setMaxRadius(1);

        timesteps[timestep]->sphere = sphere;
        //sphere->setColorBinding(colorbinding);
        sphere->setRenderMethod(coSphere::RENDER_METHOD_ARB_POINT_SPRITES);

        float *r = new float[numParticles];
        for (int n = 0; n < numParticles; n++)
        {

            int T = Ivalues[1][n];
            if (T == 0)
            {
                r[n] = 0.5;
            }
            else if (T == 1)
            {
                r[n] = 1.0;
            }
            else
            {
                r[n] = 2.0;
            }
        }

        sphere->setCoords(numParticles, xc, yc, zc, r);
        delete[] r;

        float *rc = new float[numParticles];
        float *gc = new float[numParticles];
        float *bc = new float[numParticles];
        for (int n = 0; n < numParticles; n++)
        {

            int K = values[0][n];
            int T = Ivalues[1][n];
            if (T == 0)
            {
                if (K == 12)
                {
                    rc[n] = 0.5;
                    gc[n] = 0.5;
                    bc[n] = 1.0;
                }
                else if (K > 14)
                {
                    rc[n] = 0.0;
                    gc[n] = 0.0;
                    bc[n] = 1.0;
                }
                else
                {
                    rc[n] = 0.0;
                    gc[n] = 1.0;
                    bc[n] = 0.0;
                }
            }
            else if (T == 1)
            {
                if (K == 14)
                {
                    rc[n] = 1.0;
                    gc[n] = 0.0;
                    bc[n] = 0.0;
                }
                if (K == 12)
                {
                    rc[n] = 1.0;
                    gc[n] = 1.0;
                    bc[n] = 0.0;
                }
                else if (K > 14)
                {
                    rc[n] = 1.0;
                    gc[n] = 0.0;
                    bc[n] = 1.0;
                }
                else
                {
                    rc[n] = 1.0;
                    gc[n] = 0.5;
                    bc[n] = 0.0;
                }
            }
            else
            {
                if (K == 14)
                {
                    rc[n] = 1.0;
                    gc[n] = 0.0;
                    bc[n] = 0.0;
                }
                if (K == 12)
                {
                    rc[n] = 1.0;
                    gc[n] = 1.0;
                    bc[n] = 0.0;
                }
                else if (K > 14)
                {
                    rc[n] = 1.0;
                    gc[n] = 0.0;
                    bc[n] = 1.0;
                }
                else
                {
                    rc[n] = 1.0;
                    gc[n] = 0.5;
                    bc[n] = 0.0;
                }
            }
        }
        sphere->updateColors(rc, gc, bc);

        delete[] rc;
        delete[] gc;
        delete[] bc;

        geode->addDrawable(sphere);
        switchNode->addChild(geode, true);

        delete[] xc;
        delete[] yc;
        delete[] zc;
        return numParticles;
    }
    else
    {
        return -1;
    }
    return 0;
}

void Particles::updateRadii(unsigned int valueNumber)
{
    float f = plugin->getRadius();
    for (int i = 0; i < numTimesteps; i++)
    {
        if (timesteps[i]->sphere)
        {
            int numParticles = timesteps[i]->numParticles;
            coSphere *s = timesteps[i]->sphere;
            float *rn = new float[numParticles];
            if (variableTypes[valueNumber + numHiddenVars] == T_FLOAT)
            {
                float *radii = timesteps[i]->values[variableIndex[valueNumber]];
                for (int n = 0; n < numParticles; n++)
                {
                    rn[n] = radii[n] * f;
                }
            }
            else
            {
                int64_t *radii = timesteps[i]->Ivalues[variableIndex[valueNumber]];
                for (int n = 0; n < numParticles; n++)
                {
                    rn[n] = radii[n] * f;
                }
            }

            s->updateRadii(rn);
            delete[] rn;
        }
    }
}
void Particles::updateColors(unsigned int valueNumber, unsigned int aValueNumber)
{
    for (int i = 0; i < numTimesteps; i++)
    {
        if (timesteps[i]->sphere)
        {
            int numParticles = timesteps[i]->numParticles;
            float *rc = new float[numParticles];
            float *gc = new float[numParticles];
            float *bc = new float[numParticles];
            float minVal = plugin->getMinVal();
            float maxVal = plugin->getMaxVal();
            if (variableTypes[valueNumber + numHiddenVars] == T_FLOAT)
            {
                float *temp = timesteps[i]->values[variableIndex[valueNumber]];
                for (int n = 0; n < numParticles; n++)
                {
                    osg::Vec4 c = plugin->getColor((temp[n] - minVal) / (maxVal - minVal), getMode());
                    rc[n] = c[0];
                    gc[n] = c[1];
                    bc[n] = c[2];
                }
            }
            else
            {
                int64_t *temp = timesteps[i]->Ivalues[variableIndex[valueNumber]];
                for (int n = 0; n < numParticles; n++)
                {
                    osg::Vec4 c = plugin->getColor((temp[n] - minVal) / (maxVal - minVal), getMode());
                    rc[n] = c[0];
                    gc[n] = c[1];
                    bc[n] = c[2];
                }
            }
            timesteps[i]->sphere->updateColors(rc, gc, bc);

            delete[] rc;
            delete[] gc;
            delete[] bc;
        }
        else
        {
            osg::Vec4Array *colArr;
            osg::Geometry *lines = dynamic_cast<osg::Geometry *>(timesteps[i]->lines);
            if (aValueNumber == 0) // constant Color
            {
                colArr = new osg::Vec4Array();
                colArr->push_back(lineColor);
                lines->setColorArray(colArr);
                lines->setColorBinding(osg::Geometry::BIND_OVERALL);
            }
            else
            {
                int numParticles = timesteps[i]->numParticles;
                colArr = new osg::Vec4Array(numParticles * 2);
                float minVal = plugin->getAMinVal();
                float maxVal = plugin->getAMaxVal();
                if (variableTypes[aValueNumber + numHiddenVars - 1] == T_FLOAT)
                {
                    float *temp = timesteps[i]->values[variableIndex[aValueNumber - 1]];
                    for (int n = 0; n < numParticles; n++)
                    {
                        osg::Vec4 c = plugin->getColor((temp[n] - minVal) / (maxVal - minVal), getMode());
                        colArr->at(n * 2).r() = c[0];
                        colArr->at(n * 2).g() = c[1];
                        colArr->at(n * 2).b() = c[2];
                        colArr->at(n * 2).a() = 1;
                        colArr->at(n * 2 + 1).r() = c[0];
                        colArr->at(n * 2 + 1).g() = c[1];
                        colArr->at(n * 2 + 1).b() = c[2];
                        colArr->at(n * 2 + 1).a() = 1;
                    }
                }
                else
                {
                    int64_t *temp = timesteps[i]->Ivalues[variableIndex[aValueNumber - 1]];
                    for (int n = 0; n < numParticles; n++)
                    {
                        osg::Vec4 c = plugin->getColor((temp[n] - minVal) / (maxVal - minVal), getMode());
                        colArr->at(n * 2).r() = c[0];
                        colArr->at(n * 2).g() = c[1];
                        colArr->at(n * 2).b() = c[2];
                        colArr->at(n * 2).a() = 1;
                        colArr->at(n * 2 + 1).r() = c[0];
                        colArr->at(n * 2 + 1).g() = c[1];
                        colArr->at(n * 2 + 1).b() = c[2];
                        colArr->at(n * 2 + 1).a() = 1;
                    }
                }
                lines->setColorArray(colArr);
                lines->setColorBinding(osg::Geometry::BIND_PER_VERTEX);
            }
            if (lines)
            {
                lines->dirtyDisplayList();
            }
        }
    }
}

void Particles::setTimestep(int i)
{
    //switchNode->setAllChildrenOff();
    //switchNode->setValue(i,true);

    switchNode->setSingleChildOn(i);
}

//destructor
Particles::~Particles()
{
    switchNode->getParent(0)->removeChild(switchNode);
    for (int i = 0; i < numTimesteps; i++)
        delete timesteps[i];
    delete[] timesteps;
}
