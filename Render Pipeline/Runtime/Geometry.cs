using System.Collections.Generic;
using Unity.Mathematics;
using UnityEngine;

namespace PathtracingPipeline
{
    //size  32
    public struct Vertex
    {
        public float3 pos;
        public float3 normal;
        public float2 UV;
    };
    //size 16
    public struct Triangle
    {
        public uint v0;
        public uint v1;
        public uint v2;
        public uint matID;
    };
    //size 64
    public struct MaterialData
    {
        public uint diffuseID;
        public uint specularID;
        public uint emissiveID;
        public uint normalID;

        public float roughness;
        public float metallic;
        public float refractiveIndex;
        public float opacity;

        public uint renderFlags;
        public float3 anisotropy;
    };

    public static class Geometry
    {
        public static List<Vertex> vertexData;
        public static List<Triangle> triangleData;

        public static float3 sceneMin;
        public static float3 sceneMax;
        public static void ConstructSceneData()
        {
            var vertList = new List<Vertex>();
            var triList = new List<Triangle>();

            MeshFilter[] meshFilters = GameObject.FindObjectsByType<MeshFilter>(FindObjectsSortMode.None);

            int vertOffset = 0;

            foreach (MeshFilter mf in meshFilters)
            {
                Mesh mesh = mf.sharedMesh;
                if (mesh == null) continue;

                Vector3[] positions = mesh.vertices;
                Vector3[] normals = mesh.normals;
                Vector2[] uvs = mesh.uv;
                int[] triangles = mesh.triangles;

                Transform tf = mf.transform;

                // Add transformed vertices
                for (int i = 0; i < positions.Length; i++)
                {
                    float3 worldPos = tf.TransformPoint(positions[i]);
                    float3 worldNormal = (normals.Length > i) ? tf.TransformDirection(normals[i]) : Vector3.up;
                    float2 uv = (uvs.Length > i) ? (float2)uvs[i] : float2.zero;

                    vertList.Add(new Vertex
                    {
                        pos = worldPos,
                        normal = worldNormal,
                        UV = uv
                    });

                    //gets the min and max for binning later
                    sceneMin = math.min(sceneMin, worldPos);
                    sceneMax = math.max(sceneMax, worldPos);
                }

                // Add triangles (with correct vertex offset)
                for (int i = 0; i < triangles.Length; i += 3)
                {
                    triList.Add(new Triangle
                    {
                        v0 = (uint)(triangles[i + 0] + vertOffset),
                        v1 = (uint)(triangles[i + 1] + vertOffset),
                        v2 = (uint)(triangles[i + 2] + vertOffset),
                        matID = 0 // Replace with logic if you want to assign a material ID
                    });
                }

                vertOffset += positions.Length;
            }

            vertexData = vertList;
            triangleData = triList;
        }
    }
}
