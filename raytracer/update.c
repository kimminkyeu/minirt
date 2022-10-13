/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   update.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: minkyeki <minkyeki@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2022/09/03 23:30:53 by minkyeki          #+#    #+#             */
/*   Updated: 2022/10/13 22:16:24 by minkyeki         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include <float.h> // float max
#include "gl_color.h"
#include "gl_draw.h"
#include "gl_dvec3.h"
#include "gl_vec4.h"
#include "gl_vec3.h"
#include "gl_vec2.h"
#include "libft.h"
#include "main.h"
#include "texture.h"

t_hit create_hit_data(float d, t_vec3 normal, t_vec3 point)
{
	t_hit data;

	data.distance = d;
	data.normal = normal;
	data.point = point;
	return (data);
}

// 물체의 타입에 따라 다르게 체크.
t_hit check_ray_collision(t_ray *ray, t_object *obj)
{
	if (obj->type == TYPE_SPHERE)
		return (sphere_intersect_ray_collision(ray, obj->obj_data));
	else if (obj->type == TYPE_TRIANGLE)
		return (triangle_intersect_ray_collision(ray, obj->obj_data));
	else if (obj->type == TYPE_PLAIN)
		return (plane_intersect_ray_collision(ray, obj->obj_data));
	else if (obj->type == TYPE_SQUARE)
		return (square_intersect_ray_collision(ray, obj->obj_data));
	// else if (obj->type == TYPE_CYLINDER)
	// {}
	// else if (obj->type == TYPE_CONE)
	// {}
	else
		return (create_hit_data(-1.0f, gl_vec3_1f(0.0f), gl_vec3_1f(0.0f)));
}

// device에 있는 모든 obj를 돌면서, 가장 가까운 충돌 지점을 계산.
t_hit find_closet_collision(t_device *device, t_ray *ray)
{
	float closest_distance = FLT_MAX;
	t_hit closest_hit = create_hit_data(-1.0f, gl_vec3_1f(0.0f), gl_vec3_1f(0.0f));

	size_t i = 0;
	while (i < device->objects->size)
	{
		// ... find hit
		t_hit hit = check_ray_collision(ray, device->objects->data[i]);

		// 모든 물체에 대해서 충돌을 계산하고, 그 중에서 d값이 더 작은 경우 그 값을 hit으로 만들면 된다.
		if (hit.distance >= 0.0f && hit.distance < closest_distance)
		{
			closest_distance = hit.distance;
			closest_hit = hit;
			closest_hit.obj = device->objects->data[i];

			// 텍스쳐 좌표
			closest_hit.uv = hit.uv;
		}
		i++;
	}
	return (closest_hit);
}

// screen 좌표계를 world 좌표계로 변환. (-aspect ~ +aspect)
t_vec3 transform_screen_to_world(t_image *img, t_vec2 pos_screen)
{
	const float x_scale = 2.0f / img->img_size.width;
	const float y_scale = 2.0f / img->img_size.height;
	const float aspect_ratio = (float)img->img_size.width / img->img_size.height;

	// 3차원 공간으로 확장.
	return (gl_vec3_3f((pos_screen.x * x_scale - 1.0f) * aspect_ratio, -pos_screen.y * y_scale + 1.0f, 0.0f));
}

/* ------------------------------------------------------------------------------------------- *
 * |  NOTE:  Main Ray-tracing Algorithm !   => 그림자가 자연스럽게 Blending 되려면?

 개선 가능 사항:
 (1) ambient light을 그대로 물체의 ambient_color로 사용하고 있기에, 그림자의 색이 모두 동일하게 나온다.
 --> 그림자를 처리하는 다른 방법이 있는 걸까?

 ---------------------------------------------------------------------------------------------- */
t_vec3 trace_ray(t_device *device, t_ray *ray)
{
	// (0) Render first hit
	t_hit hit = find_closet_collision(device, ray);

	if (hit.distance >= 0.0f) // if no hit.
	/**
	 * * If ray hit object, then calculate with Phong-Shading-model. */
	{
		// (1) Start with Ambient Color.
		t_vec3 point_color;

		point_color = gl_vec3_multiply_scalar(device->ambient_light->color, device->ambient_light->brightness_ratio);

		// Add texture to color (ambient texture)
		if (hit.obj->ambient_texture != NULL)  // if has texture
		{
			const t_vec3 sample_point_result = sample_point(hit.obj->ambient_texture, hit.uv); // texture sampling
			point_color.r *= sample_point_result.r;
			point_color.g *= sample_point_result.g;
			point_color.b *= sample_point_result.b;
		}

		// (2) Diffuse
		// 그림자 처리. 아주 작은 값만큼 광원을 향해 이동시켜야 hit_point로 부터 충돌처리를 피할 수 있다.
		const t_vec3 hit_point_to_light = gl_vec3_normalize(gl_vec3_subtract_vector(device->light->pos, hit.point));

		// (3) Shadow
		// 만약 [hit_point+살짝 이동한 지점] 에서  shadow_ray를 광원을 향해 쐈는데, 충돌이 감지되면 거긴 그림자로 처리.
		// WARN:  아래 그림자에서 사용된 1e-4f는 반사/반투명 물체에서 문제가 발생 할 수 있음.
		t_ray	shadow_ray = create_ray(gl_vec3_add_vector(hit.point, gl_vec3_multiply_scalar(hit_point_to_light, 1e-4f)), hit_point_to_light);
		t_hit	shadow_ray_hit = find_closet_collision(device, &shadow_ray);

		// TODO:  물체보다 광원이 더 가까운 경우, 그 경우는 그림자가 생기면 안된다.
		if (shadow_ray_hit.distance < 0.0f || shadow_ray_hit.distance > gl_vec3_get_magnitude(gl_vec3_subtract_vector(device->light->pos, hit.point)))
		{
			const float _diff = max_float(gl_vec3_dot(hit.normal, hit_point_to_light), 0.0f);

			// (3-1) Calculate Diffuse color
			t_vec3 diffuse_final = gl_vec3_multiply_scalar(hit.obj->material.diffuse, _diff);
			if (hit.obj->diffuse_texture != NULL) // if has diffuse texture
			{
				const t_vec3 sample_linear_result = sample_linear(hit.obj->diffuse_texture, hit.uv); // texture sampling
				diffuse_final.r *= sample_linear_result.r;
				diffuse_final.g *= sample_linear_result.g;
				diffuse_final.b *= sample_linear_result.b;
			}

			// (3-2) Add Diffuse color
			point_color = gl_vec3_add_vector(point_color, diffuse_final);

			// (4-1) Calculate Specular [ 2 * (N . L)N - L ]
			const t_vec3 reflection_dir = gl_vec3_subtract_vector(gl_vec3_multiply_scalar(gl_vec3_multiply_scalar(hit.normal, gl_vec3_dot(hit_point_to_light, hit.normal)), 2.0f), hit_point_to_light);
			const float _spec = pow(max_float(gl_vec3_dot(gl_vec3_reverse(ray->direction), reflection_dir), 0.0f), hit.obj->material.alpha);
			const t_vec3 specular_final = gl_vec3_multiply_scalar(gl_vec3_multiply_scalar(hit.obj->material.specular, _spec), hit.obj->material.ks);

			// (4-2) Add Specular color
			point_color = gl_vec3_add_vector(point_color, specular_final);
		}
		return (point_color);
	}
	return (gl_vec3_1f(0.0f)); // return black
}

int	update(t_device *device, t_image *img)
{
	// ray_tracing
	int	x = 0;
	int y = 0;

	while (y < img->img_size.height)
	{
		x = 0;
		while (x < img->img_size.width)
		{
			const t_vec3 pixel_pos_world = transform_screen_to_world(img, gl_vec2_2f(x, y));

			/*
			 *  NOTE:  Ray 방향 벡터. 현재 코드는 등각투시. (ray가 방향이 모두 같음. 추후 변경 필요)
			 */

			// const t_vec3 ray_dir = gl_vec3_3f(0.0f, 0.0f, 1.0f);
			const t_vec3 ray_dir = gl_vec3_normalize(gl_vec3_subtract_vector(pixel_pos_world, gl_vec3_3f(0.0f, 0.0f, -5.0f)));

			t_ray pixel_ray = create_ray(pixel_pos_world, ray_dir);
			t_vec3 tmp = gl_vec3_clamp(trace_ray(device, &pixel_ray), gl_vec3_1f(0.0f), gl_vec3_1f(255.0f));

			int final_color = gl_get_color_from_vec4(gl_vec4_4f(tmp.b, tmp.g, tmp.r, 0.0f));
			gl_draw_pixel(img, x, y, final_color);

			x++;
		}
		y++;
	}


	return (0);
}
