# mini raytracing program
### [Youtube Demo](https://youtu.be/XQrXe9Lo69E)
### [프로젝트 상세 설명 (구현, 알고리즘 소개) ](https://kyeu.notion.site/1667fc3ad2e080a1b9bbc79ce1c09048?pvs=4)

![image](https://github.com/kimminkyeu/RayTracer/assets/60287070/110d4c03-6a4b-4db1-aa1d-c3ae24ee840c)

## How to use
1. to move your camera, press mouse right button + move / WASD keys.
2. press 'r' to render high quality image

## Compile & Run
this project is for Linux, MacOS. (Windows is currently not supported)

```bash
# 1) compile with Makefile
make
# 2) run program
./minirt rt/{scene files}

# scene setting files are inside ./rt directory.
0_object_primitives.rt   # Ex) ./minirt rt/0_object_primitives.rt
0_phong shading.rt
1_multiple_lights.rt
2_without_anti_aliasing.rt
3_with_anti_aliasing.rt
5_transparent_material.rt
5_texture_earth.rt
5_texture_brick.rt
```


---
## To do List
- Global illumination
- Soft shadow
- Mimmap : 고해상도 텍스쳐 샘플링시 우글거림 해결 필요

---
