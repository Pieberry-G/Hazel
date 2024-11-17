#include "test/MXMesh.h"
#include "test/MXLight.h"
#include "test/RendererMX.h"

#include <MaterialXRender/ShaderRenderer.h>
#include <MaterialXRender/CgltfLoader.h>
#include <MaterialXRender/TinyObjLoader.h>

#include <MaterialXFormat/Environ.h>
#include <MaterialXFormat/Util.h>

#include <fstream>
#include <iostream>

namespace Hazel {

    namespace {

        void writeTextFile(const std::string& text, const std::string& filePath)
        {
            std::ofstream file;
            file.open(filePath);
            file << text;
            file.close();
        }

        void applyModifiers(mx::DocumentPtr doc, const DocumentModifiers& modifiers)
        {
            for (mx::ElementPtr elem : doc->traverseTree())
            {
                if (modifiers.remapElements.count(elem->getCategory()))
                {
                    elem->setCategory(modifiers.remapElements.at(elem->getCategory()));
                }
                if (modifiers.remapElements.count(elem->getName()))
                {
                    elem->setName(modifiers.remapElements.at(elem->getName()));
                }
                mx::StringVec attrNames = elem->getAttributeNames();
                for (const std::string& attrName : attrNames)
                {
                    if (modifiers.remapElements.count(elem->getAttribute(attrName)))
                    {
                        elem->setAttribute(attrName, modifiers.remapElements.at(elem->getAttribute(attrName)));
                    }
                }
                if (elem->hasFilePrefix() && !modifiers.filePrefixTerminator.empty())
                {
                    std::string filePrefix = elem->getFilePrefix();
                    if (!mx::stringEndsWith(filePrefix, modifiers.filePrefixTerminator))
                    {
                        elem->setFilePrefix(filePrefix + modifiers.filePrefixTerminator);
                    }
                }
                std::vector<mx::ElementPtr> children = elem->getChildren();
                for (mx::ElementPtr child : children)
                {
                    if (modifiers.skipElements.count(child->getCategory()) ||
                        modifiers.skipElements.count(child->getName()))
                    {
                        elem->removeChild(child->getName());
                    }
                }
            }

            // Remap unsupported texture coordinate indices.
            for (mx::ElementPtr elem : doc->traverseTree())
            {
                mx::NodePtr node = elem->asA<mx::Node>();
                if (node && node->getCategory() == "texcoord")
                {
                    mx::InputPtr index = node->getInput("index");
                    mx::ValuePtr value = index ? index->getValue() : nullptr;
                    if (value && value->isA<int>() && value->asA<int>() != 0)
                    {
                        index->setValue(0);
                    }
                }
            }
        }

    } // anonymous namespace


    MXMesh::MXMesh(const std::string& filePath) :
        _materialFilename("resources/Materials/Examples/StandardSurface/standard_surface_carpaint.mtlx"),
        _meshFilename(filePath),
        _meshScale(1.0f),
        _selectedGeom(0),
        _selectedMaterial(0),
        _splitByUdims(true),
        _mergeMaterials(false),
        _showAllInputs(false),
        _flattenSubgraphs(false)
    {
        createGeometryHandler();

        // Load the requested material document.
       loadDocument(RendererMX::s_Data->_stdLib);
    }

    void MXMesh::createGeometryHandler()
    {
        // Create geometry handler.
        mx::TinyObjLoaderPtr objLoader = mx::TinyObjLoader::create();
        mx::CgltfLoaderPtr gltfLoader = mx::CgltfLoader::create();
        _geometryHandler = mx::GeometryHandler::create();
        _geometryHandler->addLoader(objLoader);
        _geometryHandler->addLoader(gltfLoader);
        loadMesh(RendererMX::s_Data->_searchPath.find(_meshFilename));
    }

    void MXMesh::loadMesh(const mx::FilePath& filename)
    {
        _geometryHandler->clearGeometry();
        if (_geometryHandler->loadGeometry(filename))
        {
            _meshFilename = filename;
            if (_splitByUdims)
            {
                for (auto mesh : _geometryHandler->getMeshes())
                {
                    mesh->splitByUdims();
                }
            }

            _geometryList.clear();
            if (_geometryHandler->getMeshes().empty())
            {
                return;
            }
            for (auto mesh : _geometryHandler->getMeshes())
            {
                for (size_t partIndex = 0; partIndex < mesh->getPartitionCount(); partIndex++)
                {
                    mx::MeshPartitionPtr part = mesh->getPartition(partIndex);
                    _geometryList.push_back(part);
                }
            }

            // Assign the selected material to all geometries.
            _materialAssignments.clear();
            mx::MaterialPtr material = getSelectedMaterial();
            if (material)
            {
                for (mx::MeshPartitionPtr geom : _geometryList)
                {
                    assignMaterial(geom, material);
                }
            }

            // Unbind utility materials from the previous geometry.
            if (_wireMaterial)
            {
                _wireMaterial->unbindGeometry();
            }
            if (RendererMX::s_Data->_shadowMaterial)
            {
                RendererMX::s_Data->_shadowMaterial->unbindGeometry();
            }

            RendererMX::invalidateShadowMap();
        }
        else
        {
            //new ng::MessageDialog(this, ng::MessageDialog::Type::Warning, "Mesh Loading Error", filename);
        }
    }

    void MXMesh::applyDirectLights(mx::DocumentPtr doc)
    {
        if (RendererMX::s_Data->_light->getLightRigDoc())
        {
            doc->importLibrary(RendererMX::s_Data->_light->getLightRigDoc());
            RendererMX::s_Data->_xincludeFiles.insert(RendererMX::s_Data->_light->getLightRigFilename());
        }

        try
        {
            std::vector<mx::NodePtr> lights;
            RendererMX::s_Data->_light->getLightHandler()->findLights(doc, lights);
            RendererMX::s_Data->_light->getLightHandler()->registerLights(doc, lights, RendererMX::s_Data->_genContext);
            RendererMX::s_Data->_light->getLightHandler()->setLightSources(lights);
        }
        catch (std::exception& e)
        {
            //new ng::MessageDialog(this, ng::MessageDialog::Type::Warning, "Failed to set up lighting", e.what());
        }
    }

    void MXMesh::loadDocument(mx::DocumentPtr libraries)
    {
        const mx::FilePath& filename = _materialFilename;

        // Set up read options.
        mx::XmlReadOptions readOptions;
        readOptions.readXIncludeFunction = [](mx::DocumentPtr doc, const mx::FilePath& filename,
            const mx::FileSearchPath& searchPath, const mx::XmlReadOptions* options)
            {
                mx::FilePath resolvedFilename = searchPath.find(filename);
                if (resolvedFilename.exists())
                {
                    readFromXmlFile(doc, resolvedFilename, searchPath, options);
                }
                else
                {
                    std::cerr << "Include file not found: " << filename.asString() << std::endl;
                }
            };

        // Clear user data on the generator.
        RendererMX::s_Data->_genContext.clearUserData();

        // Clear materials if merging is not requested.
        if (!_mergeMaterials)
        {
            for (mx::MeshPartitionPtr geom : _geometryList)
            {
                if (_materialAssignments.count(geom))
                {
                    assignMaterial(geom, nullptr);
                }
            }
            _materials.clear();
        }

        std::vector<mx::MaterialPtr> newMaterials;
        try
        {
            // Load source document.
            mx::DocumentPtr doc = mx::createDocument();
            mx::readFromXmlFile(doc, filename, RendererMX::s_Data->_searchPath, &readOptions);
            _materialSearchPath = mx::getSourceSearchPath(doc);

            // Import libraries.
            doc->importLibrary(libraries);

            // Apply direct lights.
            applyDirectLights(doc);

            // Apply modifiers to the content document.
            applyModifiers(doc, RendererMX::s_Data->_modifiers);

            // Flatten subgraphs if requested.
            if (_flattenSubgraphs)
            {
                doc->flattenSubgraphs();
                for (mx::NodeGraphPtr graph : doc->getNodeGraphs())
                {
                    if (graph->getActiveSourceUri() == doc->getActiveSourceUri())
                    {
                        graph->flattenSubgraphs();
                    }
                }
            }

            // Validate the document.
            std::string message;
            if (!doc->validate(&message))
            {
                std::cerr << "*** Validation warnings for " << _materialFilename.getBaseName() << " ***" << std::endl;
                std::cerr << message;
            }

            // If requested, add implicit inputs to top-level nodes.
            if (_showAllInputs)
            {
                for (mx::NodePtr node : doc->getNodes())
                {
                    node->addInputsFromNodeDef();
                }
            }

            // Find new renderable elements.
            mx::StringVec renderablePaths;
            std::vector<mx::TypedElementPtr> elems = mx::findRenderableElements(doc);
            if (elems.empty())
            {
                throw mx::Exception("No renderable elements found in " + _materialFilename.getBaseName());
            }
            std::vector<mx::NodePtr> materialNodes;
            for (mx::TypedElementPtr elem : elems)
            {
                mx::TypedElementPtr renderableElem = elem;
                mx::NodePtr node = elem->asA<mx::Node>();
                materialNodes.push_back(node && node->getType() == mx::MATERIAL_TYPE_STRING ? node : nullptr);
                renderablePaths.push_back(renderableElem->getNamePath());
            }

            // Check for any udim set.
            mx::ValuePtr udimSetValue = doc->getGeomPropValue(mx::UDIM_SET_PROPERTY);

            // Create new materials.
            mx::TypedElementPtr udimElement;
            for (size_t i = 0; i < renderablePaths.size(); i++)
            {
                const auto& renderablePath = renderablePaths[i];
                mx::ElementPtr elem = doc->getDescendant(renderablePath);
                mx::TypedElementPtr typedElem = elem ? elem->asA<mx::TypedElement>() : nullptr;
                if (!typedElem)
                {
                    continue;
                }
                if (udimSetValue && udimSetValue->isA<mx::StringVec>())
                {
                    for (const std::string& udim : udimSetValue->asA<mx::StringVec>())
                    {
                        mx::MaterialPtr mat = RendererMX::s_Data->_renderPipeline->createMaterial();
                        mat->setDocument(doc);
                        mat->setElement(typedElem);
                        mat->setMaterialNode(materialNodes[i]);
                        mat->setUdim(udim);
                        newMaterials.push_back(mat);

                        udimElement = typedElem;
                    }
                }
                else
                {
                    mx::MaterialPtr mat = RendererMX::s_Data->_renderPipeline->createMaterial();
                    mat->setDocument(doc);
                    mat->setElement(typedElem);
                    mat->setMaterialNode(materialNodes[i]);
                    newMaterials.push_back(mat);
                }
            }

            if (!newMaterials.empty())
            {
                // Extend the image search path to include material source folders.
                mx::FileSearchPath extendedSearchPath = RendererMX::s_Data->_searchPath;
                extendedSearchPath.append(_materialSearchPath);
                RendererMX::s_Data->_imageHandler->setSearchPath(extendedSearchPath);

                // Add new materials to the global vector.
                _materials.insert(_materials.end(), newMaterials.begin(), newMaterials.end());

                mx::MaterialPtr udimMaterial = nullptr;
                for (mx::MaterialPtr mat : newMaterials)
                {
                    // Clear cached implementations, in case libraries on the file system have changed.
                    RendererMX::s_Data->_genContext.clearNodeImplementations();

                    mx::TypedElementPtr elem = mat->getElement();

                    std::string udim = mat->getUdim();
                    if (!udim.empty())
                    {
                        if ((udimElement == elem) && udimMaterial)
                        {
                            // Reuse existing material for all udims
                            mat->copyShader(udimMaterial);
                        }
                        else
                        {
                            // Generate a shader for the new material.
                            mat->generateShader(RendererMX::s_Data->_genContext);
                            if (udimElement == elem)
                            {
                                udimMaterial = mat;
                            }
                        }
                    }
                    else
                    {
                        // Generate a shader for the new material.
                        mat->generateShader(RendererMX::s_Data->_genContext);
                    }
                }

                // Apply material assignments in the order in which they are declared within the document,
                // with later assignments superseding earlier ones.
                for (mx::LookPtr look : doc->getLooks())
                {
                    for (mx::MaterialAssignPtr matAssign : look->getActiveMaterialAssigns())
                    {
                        const std::string& activeGeom = matAssign->getActiveGeom();
                        for (mx::MeshPartitionPtr part : _geometryList)
                        {
                            std::string geom = part->getName();
                            for (const std::string& id : part->getSourceNames())
                            {
                                geom += mx::ARRAY_PREFERRED_SEPARATOR + id;
                            }
                            if (mx::geomStringsMatch(activeGeom, geom, true))
                            {
                                for (mx::MaterialPtr mat : newMaterials)
                                {
                                    if (mat->getMaterialNode() == matAssign->getReferencedMaterial())
                                    {
                                        assignMaterial(part, mat);
                                        break;
                                    }
                                }
                            }
                            mx::CollectionPtr coll = matAssign->getCollection();
                            if (coll && coll->matchesGeomString(geom))
                            {
                                for (mx::MaterialPtr mat : newMaterials)
                                {
                                    if (mat->getMaterialNode() == matAssign->getReferencedMaterial())
                                    {
                                        assignMaterial(part, mat);
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                // Apply implicit udim assignments, if any.
                for (mx::MaterialPtr mat : newMaterials)
                {
                    mx::NodePtr materialNode = mat->getMaterialNode();
                    if (materialNode)
                    {
                        std::string udim = mat->getUdim();
                        if (!udim.empty())
                        {
                            for (mx::MeshPartitionPtr geom : _geometryList)
                            {
                                if (geom->getName() == udim)
                                {
                                    assignMaterial(geom, mat);
                                }
                            }
                        }
                    }
                }

                // Apply fallback assignments.
                mx::MaterialPtr fallbackMaterial = newMaterials[0];
                if (!_mergeMaterials || fallbackMaterial->getUdim().empty())
                {
                    for (mx::MeshPartitionPtr geom : _geometryList)
                    {
                        if (!_materialAssignments[geom])
                        {
                            assignMaterial(geom, fallbackMaterial);
                        }
                    }
                }
            }
        }
        catch (mx::ExceptionRenderError& e)
        {
            for (const std::string& error : e.errorLog())
            {
                std::cerr << error << std::endl;
            }
            //new ng::MessageDialog(this, ng::MessageDialog::Type::Warning, "Shader generation error", e.what());
            _materialAssignments.clear();
        }
        catch (std::exception& e)
        {
            //new ng::MessageDialog(this, ng::MessageDialog::Type::Warning, "Failed to load material", e.what());
            _materialAssignments.clear();
        }

        // Update material UI.
        updateMaterialSelections();

        RendererMX::invalidateShadowMap();
    }

    mx::ImagePtr MXMesh::getAmbientOcclusionImage(mx::MaterialPtr material)
    {
        const mx::string AO_FILENAME_SUFFIX = "_ao";
        const mx::string AO_FILENAME_EXTENSION = "png";

        if (!material || !RendererMX::s_Data->_genContext.getOptions().hwAmbientOcclusion)
        {
            return nullptr;
        }

        std::string aoSuffix = material->getUdim().empty() ? AO_FILENAME_SUFFIX : AO_FILENAME_SUFFIX + "_" + material->getUdim();
        mx::FilePath aoFilename = _meshFilename;
        aoFilename.removeExtension();
        aoFilename = aoFilename.asString() + aoSuffix;
        aoFilename.addExtension(AO_FILENAME_EXTENSION);
        return RendererMX::s_Data->_imageHandler->acquireImage(aoFilename);
    }

    mx::MaterialPtr MXMesh::getWireframeMaterial()
    {
        if (!_wireMaterial)
        {
            try
            {
                mx::ShaderPtr hwShader = mx::createConstantShader(RendererMX::s_Data->_genContext, RendererMX::s_Data->_stdLib, "__WIRE_SHADER__", mx::Color3(1.0f));
                _wireMaterial = RendererMX::s_Data->_renderPipeline->createMaterial();
                _wireMaterial->generateShader(hwShader);
            }
            catch (std::exception& e)
            {
                std::cerr << "Failed to generate wireframe shader: " << e.what() << std::endl;
                _wireMaterial = nullptr;
            }
        }

        return _wireMaterial;
    }

    void MXMesh::assignMaterial(mx::MeshPartitionPtr geometry, mx::MaterialPtr material)
    {
        if (!geometry || _geometryHandler->getMeshes().empty())
        {
            return;
        }

        if (geometry == getSelectedGeometry())
        {
            setSelectedMaterial(material);
            if (material)
            {
                //updateDisplayedProperties();
            }
        }

        if (material)
        {
            _materialAssignments[geometry] = material;
            material->unbindGeometry();
        }
        else
        {
            _materialAssignments.erase(geometry);
        }
    }

    mx::FilePath MXMesh::getBaseOutputPath()
    {
        mx::FilePath baseFilename = RendererMX::s_Data->_searchPath.find(_materialFilename);
        baseFilename.removeExtension();
        mx::FilePath outputPath = mx::getEnviron("MATERIALX_VIEW_OUTPUT_PATH");
        if (!outputPath.isEmpty())
        {
            baseFilename = outputPath / baseFilename.getBaseName();
        }
        return baseFilename;
    }

    void MXMesh::updateMaterialSelections()
    {
        std::vector<std::string> items;
        for (const auto& material : _materials)
        {
            mx::ElementPtr displayElem = material->getMaterialNode() ?
                material->getMaterialNode() :
                material->getElement();
            std::string displayName = displayElem->getName();
            if (displayName == "out")
            {
                displayName = displayElem->getParent()->getName();
            }
            if (!material->getUdim().empty())
            {
                displayName += " (" + material->getUdim() + ")";
            }
            items.push_back(displayName);
        }
    }

}