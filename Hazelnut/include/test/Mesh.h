#pragma once

#include <MaterialXRender/ShaderMaterial.h>
#include <MaterialXRender/GeometryHandler.h>

namespace Hazel {

	namespace mx = MaterialX;

	using MeshPtr = std::shared_ptr<class Mesh>;
	class Viewer;

	class Mesh
	{
	public:
		static MeshPtr create(Viewer* viewer)
		{
			return std::make_shared<Mesh>(viewer);
		}

		Mesh(Viewer* viewer);

		void createGeometryHandler();
		void loadDocument(mx::DocumentPtr libraries);

		// Return the ambient occlusion image, if any, associated with the given material.
		mx::ImagePtr getAmbientOcclusionImage(mx::MaterialPtr material);

		mx::MaterialPtr getWireframeMaterial();

		const mx::Vector3& getMeshTranslation() const { return _meshTranslation; }
		const mx::Vector3& getMeshRotation() const { return _meshRotation; }
		float getMeshScale() const { return _meshScale; }
		mx::GeometryHandlerPtr getGeometryHandler() const { return _geometryHandler; }
		const std::map<mx::MeshPartitionPtr, mx::MaterialPtr>&  getMaterialAssignments() const { return _materialAssignments; }

		void setMeshTranslation(const mx::Vector3& translation) { _meshTranslation = translation; }
		void setMeshRotation(const mx::Vector3& rotation)  { _meshRotation = rotation; }
		void setMeshScale(float scale)  { _meshScale = scale; }

		// Return true if all inputs should be shown in the property editor.
		bool getShowAllInputs() const { return _showAllInputs; }

		// Return the selected material.
		mx::MaterialPtr getSelectedMaterial() const
		{
			if (_selectedMaterial < _materials.size())
			{
				return _materials[_selectedMaterial];
			}
			return nullptr;
		}

		// Return the selected mesh partition.
		mx::MeshPartitionPtr getSelectedGeometry() const
		{
			if (_selectedGeom < _geometryList.size())
			{
				return _geometryList[_selectedGeom];
			}
			return nullptr;
		}

	private:
		void loadMesh(const mx::FilePath& filename);
		void applyDirectLights(mx::DocumentPtr doc);

		// Assign the given material to the given geometry, or remove any
		// existing assignment if the given material is nullptr.
		void assignMaterial(mx::MeshPartitionPtr geometry, mx::MaterialPtr material);

		// Mark the given material as currently selected in the viewer.
		void setSelectedMaterial(mx::MaterialPtr material)
		{
			for (size_t i = 0; i < _materials.size(); i++)
			{
				if (material == _materials[i])
				{
					_selectedMaterial = i;
					break;
				}
			}
		}

		// Generate a base output filepath for data derived from the current material.
		mx::FilePath getBaseOutputPath();

		void updateMaterialSelections();

	private:
		mx::FilePath _materialFilename;
		mx::FileSearchPath _materialSearchPath;
		mx::FilePath _meshFilename;

		mx::Vector3 _meshTranslation;
		mx::Vector3 _meshRotation;
		float _meshScale;

		// Geometry selections
		std::vector<mx::MeshPartitionPtr> _geometryList;
		size_t _selectedGeom;

		// Material selections
		std::vector<mx::MaterialPtr> _materials;
		mx::MaterialPtr _wireMaterial;
		size_t _selectedMaterial;

		// Material assignments
		std::map<mx::MeshPartitionPtr, mx::MaterialPtr> _materialAssignments;

		// Resource handlers
		mx::GeometryHandlerPtr _geometryHandler;

		// Mesh loading options
		bool _splitByUdims;

		// Material loading options
		bool _mergeMaterials;
		bool _showAllInputs;
		bool _flattenSubgraphs;

	public:
		Viewer* _viewer;
	};

}