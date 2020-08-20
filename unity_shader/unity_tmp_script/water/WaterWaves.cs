using UnityEngine;

namespace NaughtyWaterBuoyancy
{
    public class WaterWaves : MonoBehaviour
    {
        [SerializeField]
        private float speed = 1f;

        [SerializeField]
        private float height = 0.2f;

        [SerializeField]
        private float noiseWalk = 0.5f;

        [SerializeField]
        private float noiseStrength = 0.1f;

        private Mesh mesh;
        private Vector3[] baseVertices;
        private Vector3[] vertices;

        protected virtual void Awake()
        {
            this.mesh = this.GetComponent<MeshFilter>().mesh;
            this.baseVertices = this.mesh.vertices;
            this.vertices = new Vector3[this.baseVertices.Length];
        }

        protected virtual void Start()
        {
            this.ResizeBoxCollider();
        }

        public float noise_sin(float x, float y, float z)
        {
            float h = 0.0f;
            h = Mathf.Sin(Time.time * this.speed + x + y + z) *
                    (this.height / this.transform.localScale.y);
            h += Mathf.PerlinNoise(x + this.noiseWalk, y) * this.noiseStrength;
            return h;
        }

        public float water_sin(float x, float y, float z)
        {
            return Mathf.Sin(Time.time * this.speed + x);// + Mathf.Sin(Time.time * this.speed+z);
        }

        public void update_mesh_org()
        {
            for (var i = 0; i < this.vertices.Length; i++)
            {
                var vertex = this.baseVertices[i];
                float org_x = this.baseVertices[i].x;
                float org_y = this.baseVertices[i].y;
                float org_z = this.baseVertices[i].z;
                //vertex.y += noise_sin(org_x, org_y, org_z);
                vertex.y += water_sin(org_x, org_y, org_z);
                this.vertices[i] = vertex;
            }

            this.mesh.vertices = this.vertices;
            this.mesh.RecalculateNormals();
        }

        protected virtual void Update()
        {
            update_mesh_org();
        }

        private void ResizeBoxCollider()
        {
            var boxCollider = this.GetComponent<BoxCollider>();
            if (boxCollider != null)
            {
                Vector3 center = boxCollider.center;
                center.y = boxCollider.size.y / -2f;
                center.y += (this.height + this.noiseStrength) / this.transform.localScale.y;

                boxCollider.center = center;
            }
        }
    }
}
