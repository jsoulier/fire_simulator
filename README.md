### Building

Add the following to `CMakeUserPresets.json` and replace the path accordingly:

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "local",
            "inherits": "default",
            "environment": {
                "VCPKG_ROOT": "C:/<path_to_vcpkg>"
            }
        }
    ]
}
```