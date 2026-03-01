using UnityEngine;

public class CameraHandler : MonoBehaviour
{
    // Update is called once per frame
    void Update()
    {
        transform.Rotate(Input.GetAxisRaw("Mouse Y") * 2, -Input.GetAxisRaw("Mouse X") * 2, 0, Space.Self);
        transform.Translate(Input.GetAxisRaw("Horizontal") * Time.deltaTime * 10, 0, -Input.GetAxisRaw("Vertical") * Time.deltaTime * 10, Space.Self);
    }
}
