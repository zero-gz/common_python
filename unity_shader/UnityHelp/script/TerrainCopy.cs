using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class TerrainCopy : MonoBehaviour
{
    public Terrain m_terrainFrom;

    public Terrain m_terrainTo;
    // Use this for initialization
    void Start()
    {
        //m_terrainTo.terrainData.treePrototypes = null;
        m_terrainTo.terrainData.detailPrototypes = new DetailPrototype[0];
        m_terrainTo.terrainData.treeInstances = new TreeInstance[0];
        Debug.Log(m_terrainFrom.terrainData.treeInstanceCount);
        TreePrototype[] bufFrom = m_terrainFrom.terrainData.treePrototypes;
        TreeInstance[] bufIns = m_terrainFrom.terrainData.treeInstances;
        m_terrainTo.terrainData.treePrototypes = bufFrom;
        m_terrainTo.terrainData.detailPrototypes = m_terrainFrom.terrainData.detailPrototypes;
        DetailMapCopy(m_terrainFrom, m_terrainTo);
        for (int i = 0; i < bufIns.Length; i++)
        {
            m_terrainTo.AddTreeInstance(bufIns[i]);
        }
    }


    void DetailMapCopy(Terrain t, Terrain to)
    {

        var map = t.terrainData.GetDetailLayer(0, 0, t.terrainData.detailWidth, t.terrainData.detailHeight, 0);

        to.terrainData.SetDetailLayer(0, 0, 0, map);
    }

}