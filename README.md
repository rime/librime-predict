# librime-predict
librime plugin. predict next word.

## Usage
* Put the `.db` file (by default `predict.db`) in `build/` under rime's user directory.
* In `*.schema.yaml`, add `predictor` to the list of `engine/processors` at the end, after `express_editor`/`fluid_editor`;
alternatively, patch the schema with `engine/processors/+: predictor`
* Add the `prediction` switch:
```yaml
switches:
  - name: prediction
    states: [ 關閉預測, 開啓預測 ]
    reset: 1
```
* Configure your predictor:
```yaml
predictor:
  # put the `.db` predictor file in user directory/shared directory
  # defaults to 'predict.db'
  db: predict.db
  # max number of candidates for one iteration of prediction;
  # defaults to 0, which means showing all possible candidates.
  # you may want to set it the same as `page_size`, such that
  # `page_down` (especially `.` as `page_down`) does not trigger paging
  max_candidates: 5
  # max number of consecutive iterations of prediction;
  # defaults to 0, which means no limitation
  max_iterations: 1
```
* Deploy and enjoy.
