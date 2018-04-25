
<div class="collection-view">
  <?php if ($info && $schema_info): ?>
    <div class="clearfix">
      <div style="float: left;width:50%">
        <?php print render($info); ?>
      </div>
      <div style="float: left;width:50%">
        <?php print render($schema_info); ?>
      </div>
    </div>
  <?php else: ?>
    <?php print render($info); ?>
    <?php print render($schema_info); ?>
  <?php endif; ?>
    <?php print render($test_result); ?>
  <?php print render($schema); ?>
  <?php print render($unlock_form); ?>
</div>
